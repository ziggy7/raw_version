#ifndef _CONNECTION_POOL_
#define _CONNECTION_POOL_

#include <cstdio>
#include <mysql/mysql.h>
#include <list>
#include <error.h>
#include <iostream>
#include <string>
#include "../lock/locker.h"

using namespace std;

class connection_pool
{
public:
    MYSQL *GetConnection();
    bool ReleaseConnection(MYSQL *conn);
    int GetFreeConn();
    void DestroyPool();

    static connection_pool *GetInstance();

    void init(string url, string User, string PassWord,
              string DataBaseName, int Port, unsigned int MaxConn);
    connection_pool();
    ~connection_pool();

private:
    unsigned int MaxConn;
    unsigned int FreeConn;
    unsigned int CurConn;

private:
    locker lock;
    list<MYSQL *>connList;
    sem reserve;
private:
    string url;         //主机地址
    string Port;        //数据库端口号
    string User;        //登陆数据库用户名
    string PassWord;    //登陆数据库密码
    string DatabaseName;//使用数据库名
};

class connectionRAII
{
public:
    connectionRAII(MYSQL **con, connection_pool *connPool);
    ~connectionRAII();
private:
    MYSQL *conRAII;
    connection_pool* poolRAII;
};
#endif
