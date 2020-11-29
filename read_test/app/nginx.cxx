#include <iostream>
#include <cstdio>
#include <cstring>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <cstdlib>
#include <thread>
#include <pthread.h>
#include "ngx_macro.h"
#include "ngx_c_conf.h"
#include "ngx_global.h"
#include "ngx_c_func.h"
#include "ngx_c_socket.h"
#include "blocking_queue.h"
#include "ngx_c_threadpool.h"


std::mutex CConfig::Config_mutex;
CConfig::CC_Ptr CConfig::instance_ptr = nullptr;
char ** g_os_argv = nullptr;
char * gp_envmem = nullptr;
int g_environlen = 0;
blocking_queue<std::string> log_blocking_queue;
pid_t ngx_pid;
pid_t ngx_parent;
int ngx_process;
sig_atomic_t  ngx_reap;
int g_daemonized=0;
CSocekt g_socket;
CThreadPool g_threadpool;

void print_log(){
    while(true){
    std::string word = log_blocking_queue.take();
    std::cout << word << std::endl;
    const char *p = word.data();
    int n = write(ngx_log.fd, p, word.length());
    if (n == -1) 
        {
            //写失败有问题
            if(errno == ENOSPC) //写失败，且原因是磁盘没空间了
            {

            }
            else
            {
                //这是有其他错误，那么我考虑把这个错误显示到标准错误设备吧；
                if(ngx_log.fd != STDERR_FILENO) //当前是定位到文件的，则条件成立
                {
                    printf("the write is error\n");
                }
            }
        }
    }
}
void freeresource()
{
    //(1)对于因为设置可执行程序标题导致的环境变量分配的内存，我们应该释放
    if(gp_envmem)
    {
        delete []gp_envmem;
        gp_envmem = NULL;
        std::cout << "the char[] is deconstruct\n";
    }

    //(2)关闭日志文件
    if(ngx_log.fd != STDERR_FILENO && ngx_log.fd != -1)  
    {        
        close(ngx_log.fd); //不用判断结果了
        ngx_log.fd = -1; //标记下，防止被再次close吧        
    }
}
int main(int argc, char *const *argv){
    ngx_process = NGX_PROCESS_MASTER;
    ngx_pid = getpid();
    ngx_parent = getppid(); 
    g_os_argv = (char **)argv;
    gp_envmem = nullptr;
    g_environlen = 0;
    ngx_init_setproctitle();
    ngx_setproctitle("nginx: master process");
    CConfig::CC_Ptr p_config = CConfig::get_instance();
    if(p_config->Load("nginx.conf") == false){
        printf("配置文件读取发生错误\n");
        exit(1);
    }
    ngx_log_init();
    if(ngx_init_signals() != 0){
        freeresource();
        return 1;
    }
    if(g_socket.Initialize() == false){
        freeresource();
        return 1;
    }
    if(p_config->GetInt("Daemon") == 1){
        int cdaemonresult = ngx_daemon();
        if(cdaemonresult == -1){
            freeresource();
            return 1;
        }
        if(cdaemonresult == 1){
            freeresource();
            return 0;
        }
        g_daemonized = 1;
    }
    ngx_master_process_cycle();
    if(gp_envmem != NULL){
        delete[] gp_envmem;
        std::cout << "the char[] is deconstruct\n";
    }
    
    if(ngx_log.fd != STDERR_FILENO && ngx_log.fd != -1){
        close(ngx_log.fd);
        ngx_log.fd = -1;
    }
    return 0;
}
