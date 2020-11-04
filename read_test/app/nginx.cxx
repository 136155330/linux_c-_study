#include <iostream>
#include <cstdio>
#include <cstring>
#include "ngx_c_conf.h"
std::mutex CConfig::Config_mutex;
CConfig::CC_Ptr CConfig::instance_ptr = nullptr;
int main(int argc, char *const *argv){
    CConfig::CC_Ptr p_config = CConfig::get_instance();
    if(p_config->Load("nginx.conf") == false){
        printf("配置文件读取发生错误\n");
        exit(1);
    }
    std::cout << p_config->GetString("ListenPort") << std::endl;
    std::cout << p_config->GetInt("ListenPort") << std::endl;
    std::cout << p_config->GetString("DBInfo") << std::endl;
    return 0;
}