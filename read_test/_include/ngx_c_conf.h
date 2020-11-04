#ifndef __NGX_CONF_H__
#define __NGX_CONF_H__

#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <mutex>
class CConfig{
public:
    ~CConfig();
    typedef std::shared_ptr<CConfig> CC_Ptr;
    
    bool Load(const char *pconfName);
    std::string GetString(std::string name);
    int GetInt(std::string name);
private:
    CConfig(){};
    CConfig(CConfig&) = delete;
    CConfig & operator=(const CConfig&) = delete;
    static CC_Ptr instance_ptr;
    static std::mutex Config_mutex;
    std::map<std::string, std::string> m_ConfigMaplist;
public:
    static CC_Ptr get_instance(){
        if(instance_ptr == nullptr){
            std::lock_guard<std::mutex> lock(Config_mutex);
            if(instance_ptr == nullptr){
                instance_ptr = CC_Ptr(new CConfig());
            }
            return instance_ptr;
        }
    }
};

#endif