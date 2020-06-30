#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <list>
#include <cstdio>
#include <exception>
#include <pthread.h>
#include "../lock/locker.h"
#include "../CGImysql/sql_connection_pool.h"
using namespace std;
template< typename T >
class threadpool
{
public:
    /*默认8个线程，最大请求为10000*/
    threadpool(connection_pool *connPool, int thread_number = 8,int max_requests = 10000 );
    ~threadpool();
    //外部调用，所以为public
    bool append( T* request );

private:
    static void* worker( void* arg );
    void run();

private:
    int m_thread_number;
    int m_max_requests;
    /*线程指针*/
    pthread_t* m_threads;
    /*请求队列*/
    list< T* > m_workqueue;
    /*队列的锁*/
    locker m_queuelocker;
    /*队列信号量*/
    sem m_queuestat;
    bool m_stop;
    connection_pool *m_connPool;  //数据库
};

template< typename T >
threadpool<T>::threadpool( connection_pool *connPool, int thread_number, 
                        int max_requests) : m_thread_number(thread_number), 
                        m_max_requests(max_requests), m_stop(false), 
                        m_threads(NULL),m_connPool(connPool)
{
    if( ( thread_number <= 0 ) || ( max_requests <= 0 ) )
    {
        throw std::exception();
    }

    m_threads = new pthread_t[ m_thread_number ];
    if( ! m_threads )
    {
        throw std::exception();
    }

    for ( int i = 0; i < thread_number; ++i )
    {
        printf( "create the %dth thread\n", i );
        /*创建线程调用worker函数，worker函数调用run函数*/
        if( pthread_create( m_threads + i, NULL, worker, this ) != 0 )
        {
            delete [] m_threads;
            throw std::exception();
        }
        if( pthread_detach( m_threads[i] ) )
        {
            delete [] m_threads;
            throw std::exception();
        }
    }
}

template< typename T >
threadpool< T >::~threadpool()
{
    delete [] m_threads;
    m_stop = true;
}

/*插入队列*/
template< typename T >
bool threadpool< T >::append( T* request )
{
    m_queuelocker.lock();
    if ( m_workqueue.size() > m_max_requests )
    {
        m_queuelocker.unlock();
        return false;
    }
    m_workqueue.push_back( request );
    m_queuelocker.unlock();
    m_queuestat.post();
    return true;
}

/*worker要为静态函数，所以调用run*/
template< typename T >
void* threadpool< T >::worker( void* arg )
{
    threadpool* pool = ( threadpool* )arg;
    pool->run();
    return pool;
}

/*线程调用，竞争互斥锁来获得请求，取出请求并调用请求类的process函数*/
template< typename T >
void threadpool< T >::run()
{
    while ( ! m_stop )
    {
        m_queuestat.wait();
        m_queuelocker.lock();
        if ( m_workqueue.empty() )
        {
            m_queuelocker.unlock();
            continue;
        }
        T* request = m_workqueue.front();
        m_workqueue.pop_front();
        m_queuelocker.unlock();
        if ( ! request )
        {
            continue;
        }
        connectionRAII mysqlcon(&request->mysql, m_connPool);
        request->process();

    }
}

#endif

