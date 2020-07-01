#ifndef MIN_HEAP
#define MIN_HEAP

#include <iostream>
#include <netinet/in.h>
#include <time.h>

using std::exception;

//前向声明
class heap_timer;

//连接资源类
struct client_data
{
    sockaddr_in address;
    int sockfd;
    heap_timer* timer;
};

//定时器类
class heap_timer
{
public:
    heap_timer(int delay)
    {
        expire = time(NULL) + delay;
    }
public:
    time_t expire;
    //回调函数，函数指针
    void (*cb_func)(client_data*);
    client_data* user_data;
};

//时间堆类
class time_heap
{
public:
    //构造函数1:初始化大小为cap的数组
    time_heap(int cap):capacity(cap),cur_size(0)
    {
        array = new heap_timer*[capacity];
        if(!array)
            throw std::exception();
        for(int i = 0; i < capacity; ++i)
            array[i] = NULL;
    }

    //构造函数2:用已有数组来初始化堆
    time_heap(heap_timer** init_array, int size, int cap):
        cur_size(size),capacity(cap)
    {
        if(capacity < size)
            throw std::exception();
        array = new heap_timer*[capacity];
        if(!array)
            throw std::exception();
        for(int i = 0; i < capacity; ++i)
            array[i] = NULL;
        if(size != 0)
        {
            //初始化数组
            for(int i = 0; i < size; ++i)
                array[i] = init_array[i];
            //对数组中的0到(size - 1)/2个元素下滤
            for(int i = (size - 1) / 2; i >= 0; --i)
                percolate_down(i);
        }
    }

    ~time_heap()
    {
        //删除数组中的定时器
        for(int i = 0; i < cur_size; ++i)
            delete array[i];
        //删除整个数组
        delete [] array;
    }

public:
    //添加目标定时器
    void add_timer(heap_timer* timer)
    {
        if(!timer)
            return;
        //如果数组不够大，扩容
        if(cur_size >= capacity)
            resize();
        //空穴的索引为cur_size，从0开始
        int hole = cur_size;
        cur_size++;
        int parent = 0;
        //对空穴到根节点上所有节点执行上滤操作
        for(; hole > 0; hole = parent)
        {
            parent = (hole - 1) / 2;
            //大于父节点则不用调整
            if(array[parent]->expire <= timer->expire)
                break;
            //父节点下移
            array[hole] = array[parent];
        }
        array[hole] = timer;
    }

    //删除目标定时器timer
    void del_timer(heap_timer* timer)
    {
        if(!timer)
            return;
        //将目标定时器的回调函数设置为空(使定时器失效)，即延迟销毁，容易使数组膨胀
        timer->cb_func = NULL;
    }

    //获得堆顶的定时器
    heap_timer* top()const 
    {
        if(empty())
            return NULL;
        return array[0];
    }

    //删除堆顶定时器
    void pop_timer()
    {
        if(empty())
            return;
        if(array[0])
        {
            delete array[0];
            cur_size--;
            //最后元素移到0处
            array[0] = array[cur_size];
            //0处下滤
            percolate_down(0);
        }
    }
    //心搏函数
    void tick()
    {
        heap_timer* tmp = array[0];
        time_t cur = time( NULL );
        //处理失效连接
        while( !empty() )
        {
            if( !tmp )
            {
                break;
            }
            if( tmp->expire > cur )
            {
                break;
            }
            if( array[0]->cb_func )
            {
                array[0]->cb_func( array[0]->user_data );
            }
            pop_timer();
            tmp = array[0];
        }
    }
    //延长后需要调整堆
    void adjust_timer()
    {

    }
    //外部的判空
    bool empty() const { return cur_size == 0; }
     
private:
    //下滤操作
    void percolate_down( int hole )
    {
        heap_timer* temp = array[hole];
        int child = 0;
        for ( ; ((hole*2+1) <= (cur_size-1)); hole=child )
        {
            child = hole*2+1;
            if ( (child < (cur_size-1)) && (array[child+1]->expire < array[child]->expire ) )
            {
                ++child;
            }
            if ( array[child]->expire < temp->expire )
            {
                array[hole] = array[child];
            }
            else
            {
                break;
            }
        }
        array[hole] = temp;
    }

    //数组容量扩大1倍
    void resize()
    {
        heap_timer** temp = new heap_timer* [2*capacity];
        for( int i = 0; i < 2*capacity; ++i )
        {
            temp[i] = NULL;
        }
        if ( ! temp )
        {
            throw std::exception();
        }
        capacity = 2*capacity;
        for ( int i = 0; i < cur_size; ++i )
        {
            temp[i] = array[i];
        }
        delete [] array;
        array = temp;
    }

private:
    heap_timer **array; //堆数组
    int capacity;       //容量
    int cur_size;       //当前元素个数

};

#endif
