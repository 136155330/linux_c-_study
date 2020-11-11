#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>     //errno
#include <sys/stat.h>
#include <fcntl.h>

#include "ngx_global.h"
#include "ngx_c_func.h"
#include "ngx_macro.h"
#include "ngx_c_conf.h"

int ngx_daemon(){
    int pid = fork();
    if(pid == -1){//创建子进程失败
        ngx_log_error_core(NGX_LOG_EMERG,errno, "ngx_daemon()中fork()失败!");
        return -1;
    }else if(pid == 0){
        //子进程进入此处
        ngx_parent = ngx_pid;
        ngx_pid = getpid();
        if(setsid() == -1){
            ngx_log_error_core(NGX_LOG_EMERG, errno,"ngx_daemon()中setsid()失败!");
            return -1;
        }
        umask(0);
        int fd = open("/dev/null", O_RDWR);
        //dup2(int fd, int fd2)    指定一个fd来使用，若fd2已经打开，则将fd2关闭
        if (fd == -1) 
        {
            ngx_log_error_core(NGX_LOG_EMERG,errno,"ngx_daemon()中open(\"/dev/null\")失败!");        
            return -1;
        }
        if (dup2(fd, STDIN_FILENO) == -1) //先关闭STDIN_FILENO[这是规矩，已经打开的描述符，动他之前，先close]，类似于指针指向null，让/dev/null成为标准输入；
        {
            ngx_log_error_core(NGX_LOG_EMERG,errno,"ngx_daemon()中dup2(STDIN)失败!");        
            return -1;
        }
        if (dup2(fd, STDOUT_FILENO) == -1) //再关闭STDIN_FILENO，类似于指针指向null，让/dev/null成为标准输出；
        {
            ngx_log_error_core(NGX_LOG_EMERG,errno,"ngx_daemon()中dup2(STDOUT)失败!");
            return -1;
        }
        if (fd > STDERR_FILENO)  //fd应该是3，这个应该成立
        {
            if (close(fd) == -1)  //释放资源这样这个文件描述符就可以被复用；不然这个数字【文件描述符】会被一直占着；
            {
                ngx_log_error_core(NGX_LOG_EMERG,errno, "ngx_daemon()中close(fd)失败!");
                return -1;
            }
        }
        return 0;
    }else{
        return 1;
    }
}