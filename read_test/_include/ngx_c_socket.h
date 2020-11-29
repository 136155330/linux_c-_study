#ifndef __NGX_SOCKET_H__
#define __NGX_SOCKET_H__
#include <vector>
#include <list>
#include <sys/epoll.h>
#include <sys/socket.h>
#include "ngx_comm.h"
#define NGX_LISTEN_BACKLOG 511 //已完成连接队列
#define NGX_MAX_EVENTS 512 //epoll_wait一次最多接收的事件
typedef struct ngx_listening_s   ngx_listening_t, *lpngx_listening_t;
typedef struct ngx_connection_s  ngx_connection_t,*lpngx_connection_t;
typedef class  CSocekt           CSocekt;

typedef void (CSocekt::*ngx_event_handler_pt)(lpngx_connection_t c); //定义成员函数指针


struct ngx_listening_s{
    int port;//端口
    int fd;//套接字socket
    lpngx_connection_t connection; 
    //双向指向
};
//ngx_listening_s中的connection 与 ngx_connection_s中的listening
//双指向的意义何在哦
struct ngx_connection_s//连接池的节点
{
	
	int                       fd;             //套接字句柄socket
	lpngx_listening_t         listening;      //如果这个链接被分配给了一个监听套接字，那么这个里边就指向监听套接字对应的那个lpngx_listening_t的内存首地址		
    //ngx_listening_t == ngx_listening_s 大概是一个<port, fd>的结构
	//------------------------------------	
	unsigned                  instance:1;     //【位域】失效标志位：0：有效，1：失效【这个是官方nginx提供，到底有什么用，ngx_epoll_process_events()中详解】  
	uint64_t                  iCurrsequence;  //我引入的一个序号，每次分配出去时+1，此法也有可能在一定程度上检测错包废包，具体怎么用，用到了再说
	//这个标记虽然为u longlong 但是可能会存在爆掉的可能性
    struct sockaddr           s_sockaddr;     //保存对方地址信息用的
	//char                      addr_text[100]; //地址的文本信息，100足够，一般其实如果是ipv4地址，255.255.255.255，其实只需要20字节就够

	//和读有关的标志-----------------------
	//uint8_t                   r_ready;        //读准备好标记【暂时没闹明白官方要怎么用，所以先注释掉】
	uint8_t                   w_ready;        //写准备好标记

	ngx_event_handler_pt      rhandler;       //读事件的相关处理方法
	ngx_event_handler_pt      whandler;       //写事件的相关处理方法
	//和收发包相关的东西----------------------------------
	unsigned char curStat; //状态机的状态
	char dataHeadInfo[_DATA_BUFSIZE_]; //包头
	char *precvbuf; //指针 指向对应的包头写入的地址
	unsigned int irecvlen; //对应的包头还要写入多少数据

	bool ifnewrecvMem; //是否申请过内存 来存放对应的包体数据
	char *pnewMemPointer;	//指向申请的内存
	//--------------------------------------------------
	lpngx_connection_t        data;           //这是个指针【等价于传统链表里的next成员：后继指针】，指向下一个本类型对象，用于把空闲的连接池对象串起来构成一个单向链表，方便取用
};
typedef struct _STRUC_MSG_HEADER{
	lpngx_connection_t pConn; //指向port fd
	uint64_t iCurrsequence; //收到数据包时记录对应连接的序号，将来能用于比较是否连接已经作废用
}STRUC_MSG_HEADER, *LPSTRUC_MSG_HEADER;
class CSocekt{
public:
    CSocekt();
    virtual ~CSocekt();
    virtual bool Initialize();
	int  ngx_epoll_init();                                             //epoll功能初始化
	//void ngx_epoll_listenportstart();                                  //监听端口开始工作 
	int  ngx_epoll_add_event(int fd,int readevent,int writeevent,uint32_t otherflag,uint32_t eventtype,lpngx_connection_t c);     
	                                                                   //epoll增加事件
	int  ngx_epoll_process_events(int timer);
	char* outMsgRecvQueue();
private:
    void ReadConf();                                                   //专门用于读各种配置项	
	bool ngx_open_listening_sockets();                                 //监听必须的端口【支持多个端口】
	void ngx_close_listening_sockets();                                //关闭监听套接字
	bool setnonblocking(int sockfd);                                   //设置非阻塞套接字	

	//一些业务处理函数handler
	void ngx_event_accept(lpngx_connection_t oldc);                    //建立新连接
	void ngx_wait_request_handler(lpngx_connection_t c);               //设置数据来时的读处理函数
	void ngx_close_connection(lpngx_connection_t c);					//这玩意应该跟下面的close_accepted_connection一样
	void ngx_close_accepted_connection(lpngx_connection_t c);          //用户连入，我们accept4()时，得到的socket在处理中产生失败，则资源用这个函数释放【因为这里涉及到好几个要释放的资源，所以写成函数】
	
	ssize_t recvproc(lpngx_connection_t c,char *buff,ssize_t buflen);  //接收从客户端来的数据专用函数
	void ngx_wait_request_handler_proc_p1(lpngx_connection_t c);       //包头收完整后的处理，我们称为包处理阶段1：写成函数，方便复用	                                                                   
	void ngx_wait_request_handler_proc_plast(lpngx_connection_t c);    //收到一个完整包后的处理，放到一个函数中，方便调用
	void inMsgRecvQueue(char *buf, int &irmqc);                                    //收到一个完整消息后，入消息队列
	//void tmpoutMsgRecvQueue(); //临时清除对列中消息函数，测试用，将来会删除该函数
	void clearMsgRecvQueue();   

	//获取对端信息相关                                              
	size_t ngx_sock_ntop(struct sockaddr *sa,int port,u_char *text,size_t len);  //根据参数1给定的信息，获取地址端口字符串，返回这个字符串的长度

	//连接池 或 连接 相关
	lpngx_connection_t ngx_get_connection(int isock);                  //从连接池中获取一个空闲连接
	void ngx_free_connection(lpngx_connection_t c); 

private:
	int                            m_worker_connections;               //epoll连接的最大项数
	int                            m_ListenPortCount;                  //所监听的端口数量
	int                            m_epollhandle;                      //epoll_create返回的句柄

	//和连接池有关的
	lpngx_connection_t             m_pconnections;                     //注意这里可是个指针，其实这是个连接池的首地址
	lpngx_connection_t             m_pfree_connections;                //空闲连接链表头，连接池中总是有某些连接被占用，为了快速在池中找到一个空闲的连接，我把空闲的连接专门用该成员记录;
	                                                                        //【串成一串，其实这里指向的都是m_pconnections连接池里的没有被使用的成员】
	//跟连接池有关的两根指针 m_pconnecions m_pfree_connections
	//必有一根指针指向头节点，这样可以delete清内存
	//lpngx_event_t                  m_pread_events;                     //指针，读事件数组
	//lpngx_event_t                  m_pwrite_events;                    //指针，写事件数组
	int                            m_connection_n;                     //当前进程中所有连接对象的总数【连接池大小】
	int                            m_free_connection_n;                //连接池中可用连接总数

	//lpngx_listening_t 指针 指向<fd, port>
	std::vector<lpngx_listening_t> m_ListenSocketList;                 //监听套接字队列

	struct epoll_event             m_events[NGX_MAX_EVENTS];  
	size_t                         m_iLenPkgHeader;                    //sizeof(COMM_PKG_HEADER);		
	size_t                         m_iLenMsgHeader;
	std::list<char *>	m_MsgRecvQueue;

	int                            m_iRecvMsgQueueCount;               //收消息队列大小

	//多线程相关
	pthread_mutex_t                m_recvMessageQueueMutex;            //收消息队列互斥量 

};
#endif