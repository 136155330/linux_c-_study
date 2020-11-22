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

void CSocekt::ngx_wait_request_handler(lpngx_connection_t c){
    ngx_log_stderr(errno,"22222222222222222222222.");
    ngx_log_error_core(NGX_LOG_INFO,errno,"CSocekt::ngx_wait_request_handler()被触发，已经取出了对应的accept后可读"); 



    return;
}