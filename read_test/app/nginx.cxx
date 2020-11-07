#include <iostream>
#include <cstdio>
#include <cstring>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <cstdlib>
#include <thread>
#include <pthread.h>
#include "ngx_c_conf.h"
#include "ngx_global.h"
#include "ngx_c_func.h"
#include "blocking_queue.h"
std::mutex CConfig::Config_mutex;
CConfig::CC_Ptr CConfig::instance_ptr = nullptr;
char ** g_os_argv = nullptr;
char * gp_envmem = nullptr;
int g_environlen = 0;
blocking_queue<std::string> log_blocking_queue;
pid_t ngx_pid;
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
int main(int argc, char *const *argv){
    ngx_pid = getpid();
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
    std::thread t1(print_log);
    std::thread t2(print_log);
    log_blocking_queue.put(ngx_log_error_blocking_queue_core_str(5,8,"这个XXX工作的有问题,显示的结果是=%s","YYYY"));
    log_blocking_queue.put(ngx_log_error_blocking_queue_core_str(5,8,"这个XXX工作的有问题,显示的结果是=%s","hello_word"));
    log_blocking_queue.put(ngx_log_error_blocking_queue_core_str(5,8,"这个XXX工作的有问题,显示的结果是=%s","XXXX"));
    log_blocking_queue.put(ngx_log_error_blocking_queue_core_str(5,8,"这个XXX工作的有问题,显示的结果是=%s","ZZZZ"));
    sleep(5);
    while(true){
        std::cout << "test\n";
        sleep(1);
    }
    if(gp_envmem != NULL){
        delete[] gp_envmem;
        std::cout << "the char[] is deconstruct\n";
    }
    /*
    if(ngx_log.fd != STDERR_FILENO && ngx_log.fd != -1){
        close(ngx_log.fd);
        ngx_log.fd = -1;
    }
    */
    return 0;
}