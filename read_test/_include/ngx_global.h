#ifndef __NGX_GBLDEF_H__
#define __NGX_GBLDEF_H__
#include "blocking_queue.h"
#include <signal.h>
#include "ngx_c_socket.h"
#include "ngx_c_slogic.h"
#include "ngx_c_threadpool.h"
typedef struct
{
	int    log_level;   //日志级别 或者日志类型，ngx_macro.h里分0-8共9个级别
	int    fd;          //日志文件描述符

}ngx_log_t;

extern char **g_os_argv;
extern char * gp_envmem;
extern int g_environlen;
extern int g_daemonized;
extern CLogicSocket     g_socket; 
extern CThreadPool   g_threadpool;

extern pid_t       ngx_pid;
extern pid_t       ngx_parent;
extern ngx_log_t   ngx_log;
extern blocking_queue<std::string> log_blocking_queue;
extern int           ngx_process;   
extern sig_atomic_t  ngx_reap;
extern int g_stopEvent;
#endif