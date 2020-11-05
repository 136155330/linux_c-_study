#include <cstdio>
#include <cstring>
#include <unistd.h>
#include <stdlib.h>

#include "ngx_global.h"

void ngx_init_setproctitle(){
    //将对应的environ变量拷贝到新的一块内存
    int num = 0;
    while(environ[num] != NULL){
        g_environlen += (strlen(environ[num]) + 1);
        ++ num; 
    }
    gp_envmem = new char[g_environlen];
    memset(gp_envmem, 0, sizeof(gp_envmem));
    char *point_temp = gp_envmem;//临时指针
    for(int i = 0; i < num; i ++){
        size_t size = strlen(environ[i]) + 1;
        strcpy(point_temp, environ[i]);
        environ[i] = point_temp;
        point_temp += size;
    }
    return ;
}

void ngx_setproctitle(const char *title){
    size_t ititlelen = strlen(title);
    size_t e_environlen = 0;
    int num = 0;
    while(g_os_argv[num] != NULL){
        e_environlen += (strlen(g_os_argv[num]) + 1);
        ++ num;
    }
    size_t sum = e_environlen + g_environlen;
    if(sum <= ititlelen){
        return ;
    }
    g_os_argv[1] = NULL;
    char *point_temp = g_os_argv[0];
    strcpy(point_temp, title);
    point_temp += ititlelen;
    size_t sub = sum - ititlelen;
    memset(point_temp, 0, sub);
    return ;
}