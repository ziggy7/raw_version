#include <mysql/mysql.h>
#include <stdio.h>
#include <string>
#include <string.h>
#include <stdlib.h>
#include <list>
#include <pthread.h>
#include <iostream>
#include "sql_connection_pool.h"

using namespace std;

//构造函数
connection_pool::connection_pool()
{
    this->CurConn = 0;
    this->FreeConn = 0;
}

//单例模式
connection_pool *connection_pool::GetInstance()
{
    static connection_pool connPool;
    return &connPool;
}

//初始化
void connection_pool::init(string url, string User, string PassWord,
                           string DBName, int Port, unsigned int MaxConn)
{
    //初始化数据库信息
	this->url = url;
	this->Port = Port;
	this->User = User;
	this->PassWord = PassWord;
	this->DatabaseName = DBName;

    lock.lock();
    //创建MaxConn条数据库连接
    for(int i = 0; i < MaxConn; ++i)
    {
        MYSQL *con = NULL;
        con = mysql_init(con);

        if(con == NULL)
        {
            cout <<"Error:"<<mysql_errno(con);
            exit(1);
        }
        //建立连接
        //为了兼容c语言，c_str()函数返回一个
        //指向正规C字符串的指针常量, 内容与本string串相同。
        con = mysql_real_connect(con, url.c_str(),User.c_str(),
                                 PassWord.c_str(),DBName.c_str(),
                                 Port, NULL , 0);
        if(con == NULL)
        {
            cout<< "Error:"<<mysql_errno(con);
            exit(1);
        }
        connList.push_back(con);
        ++FreeConn;
    }
    //创建大小为FreeConn的信号量
    reserve = sem(FreeConn);
    this->MaxConn = FreeConn;
    lock.unlock();
}

//当有请求时，从数据库连接池返回一个可用连接，更新连接数
MYSQL *connection_pool::GetConnection()
{
    MYSQL *con = NULL;
    if(0 == connList.size())
        return NULL;

    reserve.wait();     //取出连接，信号量原子减1，为0则等待

    lock.lock();
    con = connList.front();
    connList.pop_front();

    --FreeConn;
    ++CurConn;
    lock.unlock();
    return con;
}

//释放当前使用的连接,重新加入队列
bool connection_pool::ReleaseConnection(MYSQL *con)
{
    if(con == NULL)
        return false;
    lock.lock();

    connList.push_back(con);
    ++FreeConn;
    --CurConn;
    
    lock.unlock();
    reserve.post();     //更新信号量
    return true;
}

//销毁数据库连接池
//通过迭代器遍历连接池链表，
//关闭对应数据库连接，清空链表并重置空闲连接和现有连接数量
void connection_pool::DestroyPool()
{
    lock.lock();
    if(connList.size() > 0)
    {
        list<MYSQL *>::iterator it;
        for(it = connList.begin(); it != connList.end(); ++it)
        {
            MYSQL *con = *it;
            mysql_close(con);
        }
        CurConn = 0;
        FreeConn = 0;
        connList.clear();

        lock.unlock();
    }
    //记得解锁
    lock.unlock();
}

//当前空闲的连接数
int connection_pool::GetFreeConn()
{
    return this->FreeConn;
}

//销毁连接池没有直接被外部调用，而是通过RAII机制来完成自动释放
connection_pool::~connection_pool()
{
    DestroyPool();
}

//connectionRAII构造函数,通过RAII机制进行获取和释放
connectionRAII::connectionRAII(MYSQL **SQL, connection_pool * connPool)
{
    //从池对象的getconnection的返回值获得连接
    *SQL = connPool->GetConnection();
    //printf("调用connectionRAII构造函数\n");
    conRAII = *SQL;
    poolRAII = connPool;
}

connectionRAII::~connectionRAII()
{
    poolRAII->ReleaseConnection(conRAII);
    //printf("调用connectionRAII析构函数\n");
}
