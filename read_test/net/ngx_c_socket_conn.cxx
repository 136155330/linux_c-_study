#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>    //uintptr_t
#include <stdarg.h>    //va_start....
#include <unistd.h>    //STDERR_FILENO等
#include <sys/time.h>  //gettimeofday
#include <time.h>      //localtime_r
#include <fcntl.h>     //open
#include <errno.h>     //errno
#include <sys/socket.h>
#include <sys/ioctl.h> //ioctl
#include <arpa/inet.h>
#include <fcntl.h>     //open
#include <errno.h>     //errno
#include <sys/socket.h>
#include <sys/ioctl.h> //ioctl
#include <arpa/inet.h>

#include "ngx_c_conf.h"
#include "ngx_macro.h"
#include "ngx_global.h"
#include "ngx_c_func.h"
#include "ngx_c_socket.h"
#include "ngx_c_memory.h"
#include "ngx_c_lockmutex.h"
ngx_connection_s::ngx_connection_s(){
    iCurrsequence = 0;
    pthread_mutex_init(&logicPorcMutex, NULL);
}
ngx_connection_s::~ngx_connection_s(){
    pthread_mutex_destroy(&logicPorcMutex);
}
void ngx_connection_s::GetOneToUse(){
    ++ iCurrsequence;
    curStat = _PKG_HD_INIT;
    precvbuf = dataHeadInfo;
    irecvlen = sizeof(COMM_PKG_HEADER);

    precvMemPointer = NULL;                           //既然没new内存，那自然指向的内存地址先给NULL
    iThrowsendCount = 0;                              //原子的
    psendMemPointer = NULL;                           //发送数据头指针记录
    events          = 0;                              //epoll事件先给0 
}
void ngx_connection_s::PutOneToFree(){
    ++ iCurrsequence;
    if(precvMemPointer != NULL){
        CMemory::GetInstance()->FreeMemory(precvMemPointer);
        precvMemPointer = NULL;
    }
    if(psendMemPointer != NULL){
        CMemory::GetInstance()->FreeMemory(psendMemPointer);
        psendMemPointer = NULL;
    }
    iThrowsendCount = 0;
}
void CSocekt::initconnection(){
    lpngx_connection_t p_Conn;
    CMemory *p_memory = CMemory::GetInstance();

    int ilenconnpool = sizeof(ngx_connection_t);
    for(int i = 0; i < m_worker_connections; ++ i){
        p_Conn = (lpngx_connection_t)p_memory->AllocMemory(ilenconnpool, true);
        p_Conn = new(p_Conn) ngx_connection_t();
        p_Conn->GetOneToUse();
        m_connectionList.push_back(p_Conn);
        m_freeconnectionList.push_back(p_Conn);
    }
    m_free_connection_n = m_total_connection_n = m_connectionList.size();
}
void CSocekt::clearconnection(){
    lpngx_connection_t p_Conn;
    CMemory *p_memory = CMemory::GetInstance();
    while(!m_connectionList.empty()){
        p_Conn = m_connectionList.front();
        m_connectionList.pop_front();
        p_Conn->~ngx_connection_t();
        p_memory->FreeMemory(p_Conn);
    }
}


lpngx_connection_t CSocekt::ngx_get_connection(int isock){
    CLock lock(&m_connectionMutex);
    if(!m_freeconnectionList.empty()){
        lpngx_connection_t p_Conn = m_freeconnectionList.front();
        m_freeconnectionList.pop_front();
        p_Conn->GetOneToUse();
        -- m_free_connection_n;
        p_Conn->fd = isock;
        return p_Conn;
    }
    CMemory *p_memory = CMemory::GetInstance();
    lpngx_connection_t p_Conn = (lpngx_connection_t)p_memory->AllocMemory(sizeof(ngx_connection_t), true);
    p_Conn = new(p_Conn) ngx_connection_t();
    p_Conn->GetOneToUse();
    m_connectionList.push_back(p_Conn);
    ++ m_total_connection_n;
    p_Conn->fd = isock;
    return p_Conn;
}

void CSocekt::ngx_free_connection(lpngx_connection_t pConn){
   CLock clock(&m_connectionMutex);
   pConn->PutOneToFree();
   m_freeconnectionList.push_back(pConn);
   ++ m_free_connection_n;
}

void CSocekt::inRecyConnectQueue(lpngx_connection_t pConn){
    CLock lock(&m_recyconnqueueMutex);
    pConn->inRecyTime = time(NULL);
    ++ pConn->iCurrsequence;
    m_recyconnectionList.push_back(pConn);
    ++ m_totol_recyconnection_n;
    return ;
}
void * CSocekt::ServerRecyConnectionThread(void* threadData){
    ThreadItem *pThread = static_cast<ThreadItem *>(threadData);
    CSocekt *pSocketObj = pThread->_pThis;
    time_t currtime;
    int err;
    std::list<lpngx_connection_t>::iterator pos, posend;
    lpngx_connection_t p_Conn;
    while(true){
        usleep(200 * 1000);
        if(pSocketObj->m_total_connection_n > 0){
            currtime = time(NULL);
            err = pthread_mutex_lock(&pSocketObj->m_recyconnqueueMutex);
            if(err != 0) ngx_log_stderr(err,"CSocekt::ServerRecyConnectionThread()中pthread_mutex_lock()失败，返回的错误码为%d!",err);
lblRRTD:
        pos = pSocketObj->m_recyconnectionList.begin();
        posend = pSocketObj->m_recyconnectionList.end();
        for(; pos != posend; ++ pos){
            p_Conn = (*pos);
            if((p_Conn->inRecyTime + pSocketObj->m_RecyConnectionWaitTime > currtime) && (g_stopEvent == 0)){
                continue;
            }
            --pSocketObj->m_total_connection_n;
            pSocketObj->m_recyconnectionList.erase(pos);
            pSocketObj->ngx_free_connection(p_Conn);
            goto lblRRTD;
        }
        err = pthread_mutex_unlock(&pSocketObj->m_recyconnqueueMutex);
        if(err != 0)  ngx_log_stderr(err,"CSocekt::ServerRecyConnectionThread()pthread_mutex_unlock()失败，返回的错误码为%d!",err);
        }
        if(g_stopEvent == 1){
            if(pSocketObj->m_totol_recyconnection_n > 0){
                err = pthread_mutex_lock(&pSocketObj->m_recyconnqueueMutex);  
                if(err != 0) ngx_log_stderr(err,"CSocekt::ServerRecyConnectionThread()中pthread_mutex_lock2()失败，返回的错误码为%d!",err);
lblRRTD2:
                pos    = pSocketObj->m_recyconnectionList.begin();
			    posend = pSocketObj->m_recyconnectionList.end();
                for(; pos != posend; ++pos)
                {
                    p_Conn = (*pos);
                    --pSocketObj->m_totol_recyconnection_n;        //待释放连接队列大小-1
                    pSocketObj->m_recyconnectionList.erase(pos);   //迭代器已经失效，但pos所指内容在p_Conn里保存着呢
                    pSocketObj->ngx_free_connection(p_Conn);	   //归还参数pConn所代表的连接到到连接池中
                    goto lblRRTD2; 
                } //end for
                err = pthread_mutex_unlock(&pSocketObj->m_recyconnqueueMutex); 
                if(err != 0)  ngx_log_stderr(err,"CSocekt::ServerRecyConnectionThread()pthread_mutex_unlock2()失败，返回的错误码为%d!",err);
            }
            break;
        }
    }
    return (void *)0;
}
void CSocekt::ngx_close_connection(lpngx_connection_t pConn){
    ngx_free_connection(pConn);
    if(close(pConn->fd) == -1){
        ngx_log_error_core(NGX_LOG_ALERT,errno,"CSocekt::ngx_close_connection()中close(%d)失败!",pConn->fd);  
    }
    return ;
}