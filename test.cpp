#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <cassert>
#include <sys/epoll.h>

#include "lock/locker.h"
#include "threadpool/threadpool.h"
#include "http_conn/http_conn.h"
#include "timer/lst_timer.h"
#include "./log/log.h"

#define MAX_FD 65536
#define mAX_EVENT_NUMBER 10000
#define TIMESLOT 5

#define SYNSQL
