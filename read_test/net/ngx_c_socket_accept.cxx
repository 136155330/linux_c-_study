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


void CSocekt::ngx_event_accept(lpngx_connection_t oldc){
    ngx_log_error_core(NGX_LOG_INFO,0,"来了一个监听事件!");
    struct sockaddr mysockaddr;
    socklen_t socklen;
    int err;
    int level;
    int s;
    static int use_acceptd4 = 1;
    lpngx_connection_t newc;
    socklen = sizeof(mysockaddr);
    while(true){
        if(use_acceptd4){
            s = accept4(oldc->fd, &mysockaddr, &socklen, SOCK_NONBLOCK); //从内核获取一个用户端连接，最后一个参数SOCK_NONBLOCK表示返回一个非阻塞的socket，节省一次ioctl【设置为非阻塞】调用
        }
        else{
            s = accept(oldc->fd, &mysockaddr, &socklen);
        }
        if(s == -1){
            err = errno;
            if(err == EAGAIN){
                return ;
            }
            level = NGX_LOG_ALERT;
            if(err == ECONNABORTED){
                level = NGX_LOG_ERR;
            }
            else if(err == EMFILE || err == ENFILE){
                level = NGX_LOG_CRIT;
            }
            ngx_log_error_core(level,errno,"CSocekt::ngx_event_accept()中accept4()失败!");
            if(use_acceptd4 && err == ENOSYS){
                use_acceptd4 = 0;
                continue;
            }
            if(err == ECONNABORTED){

            }
            if(err == EMFILE || err == ENFILE){

            }
            return ;
        }
        newc = ngx_get_connection(s);
        if(newc == NULL){
            if(close(s) == -1){
                ngx_log_error_core(NGX_LOG_ALERT,errno,"CSocekt::ngx_event_accept()中close(%d)失败!",s);                
            }
            return ;
        }
        memcpy(&newc->s_sockaddr, &mysockaddr, socklen);
        if(!use_acceptd4){
            if(setnonblocking(s) == false){
                ngx_close_accepted_connection(newc);
                return ;
            }
        }
        newc->listening = oldc->listening;
        newc->w_ready = 1;
        newc->rhandler = &CSocekt::ngx_wait_request_handler;
         if(ngx_epoll_add_event(s,                 //socket句柄
                                1,0,              //读，写 ,这里读为1，表示客户端应该主动给我服务器发送消息，我服务器需要首先收到客户端的消息；
                                0,          //其他补充标记【EPOLLET(高速模式，边缘触发ET)】
                                EPOLL_CTL_ADD,    //事件类型【增加，还有删除/修改】                                    
                                newc              //连接池中的连接
                                ) == -1)
        {
            //增加事件失败，失败日志在ngx_epoll_add_event中写过了，因此这里不多写啥；
            ngx_close_accepted_connection(newc);//回收连接池中的连接（千万不能忘记），并关闭socket
            return; //直接返回
        }
        break;
    }
    return ;
}

void CSocekt::ngx_close_accepted_connection(lpngx_connection_t c)
{
    int fd = c->fd;
    ngx_free_connection(c);
    c->fd = -1;
    if(close(fd) == -1){
        ngx_log_error_core(NGX_LOG_ALERT,errno,"CSocekt::ngx_close_accepted_connection()中close(%d)失败!",fd);  
    }
    return ;
}