#ifndef __NGX_SOCKET_H__
#define __NGX_SOCKET_H__
#include <vector>
#define NGX_LISTEN_BACKLOG 511

typedef struct ngx_listening_s
{
    int port;
    int fd;
}ngx_listening_t,*lpngx_listening_t;


class CSocekt{
public:
    CSocekt();
    virtual ~CSocekt();
public:
    virtual bool Initialize();
private:
    bool ngx_open_listening_sockets();                    //监听必须的端口【支持多个端口】
	void ngx_close_listening_sockets();                   //关闭监听套接字
	bool setnonblocking(int sockfd);
private:
    int m_ListenPortCount;
    std::vector<lpngx_listening_t> m_ListenSocketList;
};



#endif