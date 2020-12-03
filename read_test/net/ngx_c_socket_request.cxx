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

void CSocekt::ngx_wait_request_handler(lpngx_connection_t c){
    ngx_log_error_core(NGX_LOG_INFO,0,"ngx_wait_request_handler 运行");
    ssize_t reco = recvproc(c, c->precvbuf, c->irecvlen);
    if(reco <= 0){
        ngx_log_error_core(NGX_LOG_INFO,0,"reco <= 0");
        return ;
    }
    if(c->curStat == _PKG_HD_INIT){
        ngx_log_error_core(NGX_LOG_INFO,0,"c->curstat = _PKG_HD_INIT");
        if(reco == m_iLenPkgHeader){
            ngx_wait_request_handler_proc_p1(c);
        }else{
            ngx_log_error_core(NGX_LOG_INFO,0,"包头没有收齐");
            c->curStat = _PKG_HD_RECVING;
            c->precvbuf = c->precvbuf + reco;
            c->irecvlen = c->irecvlen - reco;
        }
    }else if(c->curStat == _PKG_HD_RECVING){
        if(c->irecvlen == reco){
            ngx_wait_request_handler_proc_p1(c); //那就调用专门针对包头处理完整的函数去处理把。
        }else{
            c->precvbuf = c->precvbuf + reco;
            c->irecvlen = c->irecvlen - reco;
        }
    }else if(c->curStat == _PKG_BD_INIT){
        if(reco == c->irecvlen){
            ngx_wait_request_handler_proc_plast(c);
        }else{
            c->curStat = _PKG_BD_RECVING;
            c->precvbuf = c->precvbuf + reco;
            c->irecvlen = c->irecvlen - reco;
        }
    }else if(c->curStat == _PKG_BD_RECVING){
        if(c->irecvlen == reco){
            ngx_wait_request_handler_proc_plast(c);
        }else{
            c->precvbuf = c->precvbuf + reco;
            c->irecvlen = c->irecvlen - reco;
        }
    }
    return;
}


ssize_t CSocekt::recvproc(lpngx_connection_t c,char *buff,ssize_t buflen)  //ssize_t是有符号整型，在32位机器上等同与int，在64位机器上等同与long int，size_t就是无符号型的ssize_t
{
    ssize_t n;
    
    n = recv(c->fd, buff, buflen, 0); //recv()系统函数， 最后一个参数flag，一般为0；     
    if(n == 0)
    {
        //客户端关闭【应该是正常完成了4次挥手】，我这边就直接回收连接连接，关闭socket即可 
        //ngx_log_stderr(0,"连接被客户端正常关闭[4路挥手关闭]！");
        ngx_close_connection(c);
        return -1;
    }
    //客户端没断，走这里 
    if(n < 0) //这被认为有错误发生
    {
        //EAGAIN和EWOULDBLOCK[【这个应该常用在hp上】应该是一样的值，表示没收到数据，一般来讲，在ET模式下会出现这个错误，因为ET模式下是不停的recv肯定有一个时刻收到这个errno，但LT模式下一般是来事件才收，所以不该出现这个返回值
        if(errno == EAGAIN || errno == EWOULDBLOCK)
        {
            //我认为LT模式不该出现这个errno，而且这个其实也不是错误，所以不当做错误处理
            ngx_log_stderr(errno,"CSocekt::recvproc()中errno == EAGAIN || errno == EWOULDBLOCK成立，出乎我意料！");//epoll为LT模式不应该出现这个返回值，所以直接打印出来瞧瞧
            return -1; //不当做错误处理，只是简单返回
        }
        //EINTR错误的产生：当阻塞于某个慢系统调用的一个进程捕获某个信号且相应信号处理函数返回时，该系统调用可能返回一个EINTR错误。
        //例如：在socket服务器端，设置了信号捕获机制，有子进程，当在父进程阻塞于慢系统调用时由父进程捕获到了一个有效信号时，内核会致使accept返回一个EINTR错误(被中断的系统调用)。
        if(errno == EINTR)  //这个不算错误，是我参考官方nginx，官方nginx这个就不算错误；
        {
            //我认为LT模式不该出现这个errno，而且这个其实也不是错误，所以不当做错误处理
            ngx_log_stderr(errno,"CSocekt::recvproc()中errno == EINTR成立，出乎我意料！");//epoll为LT模式不应该出现这个返回值，所以直接打印出来瞧瞧
            return -1; //不当做错误处理，只是简单返回
        }

        //所有从这里走下来的错误，都认为异常：意味着我们要关闭客户端套接字要回收连接池中连接；

        //errno参考：http://dhfapiran1.360drm.com        
        if(errno == ECONNRESET)  //#define ECONNRESET 104 /* Connection reset by peer */
        {
            //如果客户端没有正常关闭socket连接，却关闭了整个运行程序【真是够粗暴无理的，应该是直接给服务器发送rst包而不是4次挥手包完成连接断开】，那么会产生这个错误            
            //10054(WSAECONNRESET)--远程程序正在连接的时候关闭会产生这个错误--远程主机强迫关闭了一个现有的连接
            //算常规错误吧【普通信息型】，日志都不用打印，没啥意思，太普通的错误
            //do nothing

            //....一些大家遇到的很普通的错误信息，也可以往这里增加各种，代码要慢慢完善，一步到位，不可能，很多服务器程序经过很多年的完善才比较圆满；
        }
        else
        {
            //能走到这里的，都表示错误，我打印一下日志，希望知道一下是啥错误，我准备打印到屏幕上
            ngx_log_stderr(errno,"CSocekt::recvproc()中发生错误，我打印出来看看是啥错误！");  //正式运营时可以考虑这些日志打印去掉
        } 
        
        //ngx_log_stderr(0,"连接被客户端 非 正常关闭！");

        //这种真正的错误就要，直接关闭套接字，释放连接池中连接了
        //ngx_close_connection(c);
        if(close(c->fd) == -1){
            ngx_log_error_core(NGX_LOG_ALERT,errno,"CSocekt::recvproc()中close_2(%d)失败!",c->fd);  
        }
        inRecyConnectQueue(c);
        return -1;
    }

    //能走到这里的，就认为收到了有效数据
    return n; //返回收到的字节数
}

void CSocekt::ngx_wait_request_handler_proc_p1(lpngx_connection_t c)
{
    ngx_log_error_core(NGX_LOG_INFO,0,"ngx_wait_request_handler_proc_p1 运行");
    CMemory *p_memory = CMemory::GetInstance();
    LPCOMM_PKG_HEADER pPkgHeader;
    pPkgHeader = (LPCOMM_PKG_HEADER)c->dataHeadInfo;
    unsigned short e_pkgLen;
    e_pkgLen = ntohs(pPkgHeader->pkgLen);
    if(e_pkgLen < m_iLenPkgHeader){
        c->curStat = _PKG_HD_INIT;
        c->precvbuf = c->dataHeadInfo;
        c->irecvlen = m_iLenPkgHeader;
    }else if(e_pkgLen > (_PKG_MAX_LENGTH - 1000)){
        c->curStat = _PKG_HD_INIT;
        c->precvbuf = c->dataHeadInfo;
        c->irecvlen = m_iLenPkgHeader;
    }else{
        char *pTmpBuffer  = (char *)p_memory->AllocMemory(m_iLenMsgHeader + e_pkgLen,false); //分配内存【长度是 消息头长度  + 包头长度 + 包体长度】，最后参数先给false，表示内存不需要memset;
        c->precvMemPointer = pTmpBuffer;
        LPSTRUC_MSG_HEADER ptmpMsgHeader = (LPSTRUC_MSG_HEADER)pTmpBuffer;
        ptmpMsgHeader->pConn = c;
        ptmpMsgHeader->iCurrsequence = c->iCurrsequence;
        pTmpBuffer += m_iLenMsgHeader;
        memcpy(pTmpBuffer,pPkgHeader,m_iLenPkgHeader);
        if(e_pkgLen == m_iLenPkgHeader)
        {
            //该报文只有包头无包体【我们允许一个包只有包头，没有包体】
            //这相当于收完整了，则直接入消息队列待后续业务逻辑线程去处理吧
            ngx_wait_request_handler_proc_plast(c);
        } 
        else
        {
            //开始收包体，注意我的写法
            c->curStat = _PKG_BD_INIT;                   //当前状态发生改变，包头刚好收完，准备接收包体	    
            c->precvbuf = pTmpBuffer + m_iLenPkgHeader;  //pTmpBuffer指向包头，这里 + m_iLenPkgHeader后指向包体 weizhi
            c->irecvlen = e_pkgLen - m_iLenPkgHeader;    //e_pkgLen是整个包【包头+包体】大小，-m_iLenPkgHeader【包头】  = 包体
        }      
    }
    return ;
}
void CSocekt::ngx_wait_request_handler_proc_plast(lpngx_connection_t c)
{
    int irmqc = 0;
    ngx_log_error_core(NGX_LOG_INFO,0,"ngx_wait_request_handler_proc_plast 运行");
    //inMsgRecvQueue(c->pnewMemPointer, irmqc);
    g_threadpool.inMsgRecvQueueAndSignal(c->precvMemPointer);
    c->precvMemPointer = NULL;
    c->curStat = _PKG_HD_INIT;
    c->precvbuf = c->dataHeadInfo;
    c->irecvlen = m_iLenPkgHeader;
    return ;
}
ssize_t CSocekt::sendproc(lpngx_connection_t c,char *buff,ssize_t size)  //ssize_t是有符号整型，在32位机器上等同与int，在64位机器上等同与long int，size_t就是无符号型的ssize_t
{
    //这里参考借鉴了官方nginx函数ngx_unix_send()的写法
    ssize_t   n;

    for ( ;; )
    {

        n = send(c->fd, buff, size, 0); //send()系统函数， 最后一个参数flag，一般为0； 
        if(n > 0) //成功发送了一些数据
        {        
            //发送成功一些数据，但发送了多少，我们这里不关心，也不需要再次send
            //这里有两种情况
            //(1) n == size也就是想发送多少都发送成功了，这表示完全发完毕了
            //(2) n < size 没法送完毕，那肯定是发送缓冲区满了，所以也不必要重试发送，直接返回吧
            return n; //返回本次发送的字节数
        }

        if(n == 0)
        {
            //send()返回0？ 一般recv()返回0表示断开,send()返回0，当做正常处理吧；我个人认为send()返回0，要么你发送的字节是0，要么对端可能断开。
            //网上找资料：send=0表示超时，对方主动关闭了连接过程
            //遵循一个原则，连接断开，我们并不在send动作里处理，集中到recv那里处理，否则send,recv都处理都处理连接断开会乱套
            //连接断开epoll会通知并且 recvproc()里会处理，不在这里处理
            return 0;
        }

        if(errno == EAGAIN)  //这东西应该等于EWOULDBLOCK
        {
            //内核缓冲区满，这个不算错误
            return -1;  //表示发送缓冲区满了
        }

        if(errno == EINTR) 
        {
            //这个应该也不算错误 ，收到某个信号导致send产生这个错误？
            //参考官方的写法，打印个日志，其他啥也没干，那就是等下次for循环重新send试一次了
            ngx_log_stderr(errno,"CSocekt::sendproc()中send()失败.");  //打印个日志看看啥时候出这个错误
            //其他不需要做什么，等下次for循环吧            
        }
        else
        {
            //走到这里表示是其他错误码，都表示错误，错误我也不断开socket，我也依然等待recv()来统一处理断开，因为我是多线程，send()也处理断开，recv()也处理断开，很难处理好
            return -2;    
        }
    } //end for
}
void CSocekt::threadRecvProcFunc(char *pMsgBuf)
{   
    return;
}
// void CSocekt::inMsgRecvQueue(char *buf, int &irmqc){
//     CLock lock(&m_recvMessageQueueMutex);
//     m_MsgRecvQueue.push_back(buf);
//     ++m_iRecvMsgQueueCount;
//     irmqc = m_iRecvMsgQueueCount;
//     //为了测试方便，因为本函数意味着收到了一个完整的数据包，所以这里打印一个信息
//     ngx_log_error_core(NGX_LOG_INFO,0,"非常好，收到了一个完整的数据包【包头+包体】！");
//     return ;
// }
// char* CSocekt::outMsgRecvQueue(){
//     CLock lock(&m_recvMessageQueueMutex);
//     if(m_MsgRecvQueue.empty())  return NULL;
//     char *result = m_MsgRecvQueue.front();
//     m_MsgRecvQueue.pop_front();
//     --m_iRecvMsgQueueCount;
//     return result;
// }