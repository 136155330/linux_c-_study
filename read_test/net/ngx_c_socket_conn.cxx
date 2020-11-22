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

lpngx_connection_t CSocekt::ngx_get_connection(int isock){
    lpngx_connection_t c = m_pfree_connections;
    if(c == NULL){
        ngx_log_stderr(0,"CSocekt::ngx_get_connection()中空闲链表为空,这不应该!");
        return NULL;
    }
    m_pfree_connections = c->data;
    -- m_free_connection_n;
    uintptr_t instance = c->instance;
    uint64_t iCurrsequence = c->iCurrsequence;
    memset(c, 0, sizeof(ngx_connection_t));
    c->fd = isock;
    c->instance = !instance;
    c->iCurrsequence = iCurrsequence;
    ++c->iCurrsequence;
    return c;
}

void CSocekt::ngx_free_connection(lpngx_connection_t c){
    c->data = m_pfree_connections;
    ++c->iCurrsequence;
    m_pfree_connections = c;
    ++ m_free_connection_n;
    return ;
}