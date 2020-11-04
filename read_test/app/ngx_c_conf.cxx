#include <cstdio>
#include <cstring>
#include <iostream>
#include "ngx_c_conf.h"

CConfig::~CConfig(){
    std::cout << "CConfig::~CConfig is start\n" << std::endl;
}

bool CConfig::Load(const char *pconfName){
    FILE *fp;
    fp = fopen(pconfName, "r");
    if(fp == NULL){
        return false;
    }
    char linebuf[501];
    while(!feof(fp)){
        if(fgets(linebuf, 500, fp) != NULL){
            //printf("%s\n", linebuf);
            int len = strlen(linebuf);
            if(len == 0){
                continue;
            }
            std::string key = "";
            std::string value = "";
            if(linebuf[0] == ' ' || linebuf[0] == ';' || linebuf[0] == '\n' || linebuf[0] == '\t' || linebuf[0] == '[' || linebuf[0] == '#')  continue;
            int i = 0;
            int flag = 0;
            for(; i < len; i ++){
                if(linebuf[i] == '='){
                    break;
                }
                else if(linebuf[i] == ' '|| linebuf[i] == '\n' || linebuf[i] == '\0' || linebuf[i] == '\r'){
                    continue;
                }else{
                    if(linebuf[i] >= '0' && linebuf[i] <= '9') flag ++;
                    if(linebuf[i] >= 'a' && linebuf[i] <= 'z') flag ++;
                    if(linebuf[i] >= 'A' && linebuf[i] <= 'Z') flag ++;
                    key += linebuf[i];
                }
            }
            i += 1;
            for(; i < len; i ++){
                if(linebuf[i] == ' '|| linebuf[i] == '\n' || linebuf[i] == '\0' || linebuf[i] == '\r'){
                    continue;
                }else{
                    if(linebuf[i] >= '0' && linebuf[i] <= '9') flag ++;
                    if(linebuf[i] >= 'a' && linebuf[i] <= 'z') flag ++;
                    if(linebuf[i] >= 'A' && linebuf[i] <= 'Z') flag ++;
                    value += linebuf[i];
                }
            }/*
            std::cout << "key = ";
            std::cout << key << std::endl;
            std::cout << "value = ";
            std::cout << value << std::endl;*/
            if(flag > 0)
                m_ConfigMaplist[key] = value;
        }
    }
    fclose(fp);
    return true;
}
std::string CConfig::GetString(std::string name){
    if(m_ConfigMaplist.count(name) != 0){
        return m_ConfigMaplist[name];
    }else{
        return "NULL";
    }
}
int CConfig::GetInt(std::string name){
    if(m_ConfigMaplist.count(name) != 0){
        int temp = 0;
        std::string str = m_ConfigMaplist[name];
        for(int i = 0; i < str.length(); i ++){
            if(!(str[i] >= '0' && str[i] <= '9')){
                return -1;
            }
            temp = temp * 10 + (str[i] - '0');
        }
        return temp;
    }else{
        return -1;
    }
}