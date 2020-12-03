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
//#include <sys/socket.h>
#include <sys/ioctl.h> //ioctl
#include <arpa/inet.h>
#include <pthread.h>   //多线程

#include "ngx_c_conf.h"
#include "ngx_macro.h"
#include "ngx_global.h"
#include "ngx_c_func.h"
#include "ngx_c_memory.h"
#include "ngx_logiccomm.h"
#include "ngx_c_crc32.h"
#include "ngx_c_slogic.h"

//定义成员函数指针
typedef bool (CLogicSocket::*handler)(  lpngx_connection_t pConn,      //连接池中连接的指针
                                        LPSTRUC_MSG_HEADER pMsgHeader,  //消息头指针
                                        char *pPkgBody,                 //包体指针
                                        unsigned short iBodyLength);    //包体长度
static const handler statusHandler[]={
    NULL,                                                   //【0】：下标从0开始
    NULL,                                                   //【1】：下标从0开始
    NULL,                                                   //【2】：下标从0开始
    NULL,                                                   //【3】：下标从0开始
    NULL,
    &CLogicSocket::_HandleRegister,
    &CLogicSocket::_HandleLogIn,
};
#define AUTH_TOTAL_COMMANDS sizeof(statusHandler)/sizeof(handler) //整个命令有多少个，编译时即可知道
CLogicSocket::CLogicSocket(){

}
CLogicSocket::~CLogicSocket(){

}
bool CLogicSocket::Initalize(){
    bool bParentInit = CSocekt::Initialize();
    return bParentInit;
}
void CLogicSocket::threadRecvProcFunc(char *pMsgBuf){
    LPSTRUC_MSG_HEADER pMsgHeader = (LPSTRUC_MSG_HEADER)pMsgBuf;                  //消息头
    LPCOMM_PKG_HEADER  pPkgHeader = (LPCOMM_PKG_HEADER)(pMsgBuf+m_iLenMsgHeader); //包头
    void  *pPkgBody = NULL;                                                       //指向包体的指针
    unsigned short pkglen = ntohs(pPkgHeader->pkgLen);                            //客户端指明的包宽度【包头+包体】
    if(m_iLenPkgHeader == pkglen){
        if(pPkgHeader->crc32 != 0)  return ;
        pPkgBody = NULL;
    }else{
        pPkgHeader->crc32 = ntohl(pPkgHeader->crc32);
        pPkgBody = (void *)(pMsgBuf + m_iLenMsgHeader + m_iLenPkgHeader);
        int calccrc = CCRC32::GetInstance()->Get_CRC((unsigned char *)pPkgBody,pkglen-m_iLenPkgHeader);
        if(calccrc != pPkgHeader->crc32){
            ngx_log_error_core(NGX_LOG_INFO,0,"CLogicSocket::threadRecvProcFunc()中CRC错误，丢弃数据!");
            //ngx_log_stderr(0,"CLogicSocket::threadRecvProcFunc()中CRC错误，丢弃数据!");    //正式代码中可以干掉这个信息
			return; //crc错，直接丢弃
        }
    }
    unsigned short imsgCode = ntohs(pPkgHeader->msgCode);
    lpngx_connection_t p_Conn = pMsgHeader->pConn;
    if(p_Conn->iCurrsequence != pMsgHeader->iCurrsequence)  return ;
    if(imsgCode >= AUTH_TOTAL_COMMANDS){
        ngx_log_error_core(NGX_LOG_INFO,0,"CLogicSocket::threadRecvProcFunc()中imsgCode=%d消息码不对!", imsgCode);
        //ngx_log_stderr(0,"CLogicSocket::threadRecvProcFunc()中imsgCode=%d消息码不对!",imsgCode); //这种有恶意倾向或者错误倾向的包，希望打印出来看看是谁干的
        return; //丢弃不理这种包【恶意包或者错误包】
    }
    if(statusHandler[imsgCode] == NULL){
        ngx_log_error_core(NGX_LOG_INFO,0,"CLogicSocket::threadRecvProcFunc()中imsgCode=%d消息码找不到对应的处理函数!", imsgCode);
        //ngx_log_stderr(0,"CLogicSocket::threadRecvProcFunc()中imsgCode=%d消息码找不到对应的处理函数!",imsgCode); //这种有恶意倾向或者错误倾向的包，希望打印出来看看是谁干的
        return;  //没有相关的处理函数
    }
    (this->*statusHandler[imsgCode])(p_Conn,pMsgHeader,(char *)pPkgBody,pkglen-m_iLenPkgHeader);
    return ;
}
bool CLogicSocket::_HandleRegister(lpngx_connection_t pConn,LPSTRUC_MSG_HEADER pMsgHeader,char *pPkgBody,unsigned short iBodyLength)
{
    ngx_log_error_core(NGX_LOG_INFO,0,"执行了CLogicSocket::_HandleRegister()!");
    //ngx_log_stderr(0,"执行了CLogicSocket::_HandleRegister()!");
    return true;
}
bool CLogicSocket::_HandleLogIn(lpngx_connection_t pConn,LPSTRUC_MSG_HEADER pMsgHeader,char *pPkgBody,unsigned short iBodyLength)
{
    ngx_log_error_core(NGX_LOG_INFO,0,"执行了CLogicSocket::_HandleLogIn()!");
    //ngx_log_stderr(0,"执行了CLogicSocket::_HandleLogIn()!");
    return true;
}