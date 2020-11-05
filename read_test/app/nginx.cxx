#include <iostream>
#include <cstdio>
#include <cstring>
#include <time.h>
#include <unistd.h>
#include <cstdlib>
#include "ngx_c_conf.h"
#include "ngx_global.h"
#include "ngx_c_func.h"
std::mutex CConfig::Config_mutex;
CConfig::CC_Ptr CConfig::instance_ptr = nullptr;
char ** g_os_argv = nullptr;
char * gp_envmem = nullptr;
int g_environlen = 0;
int main(int argc, char *const *argv){
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
    std::cout << p_config->GetString("ListenPort") << std::endl;
    std::cout << p_config->GetInt("ListenPort") << std::endl;
    std::cout << p_config->GetString("DBInfo") << std::endl;
    /*while(true){
        std::cout << "test\n";
        sleep(1);
    }*/
    if(gp_envmem != NULL){
        delete[] gp_envmem;
        std::cout << "the char[] is deconstruct\n";
    }
    return 0;
}