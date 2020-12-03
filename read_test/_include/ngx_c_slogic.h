#ifndef __NGX_C_SLOGIC_H__
#define __NGX_C_SLOGIC_H__

#include <sys/socket.h>
#include "ngx_c_socket.h"
class CLogicSocket : public CSocekt{
public:
    CLogicSocket();
    virtual ~CLogicSocket();
    virtual bool Initalize();
public:
	//各种业务逻辑相关函数都在之类
	bool _HandleRegister(lpngx_connection_t pConn,LPSTRUC_MSG_HEADER pMsgHeader,char *pPkgBody,unsigned short iBodyLength);
	bool _HandleLogIn(lpngx_connection_t pConn,LPSTRUC_MSG_HEADER pMsgHeader,char *pPkgBody,unsigned short iBodyLength);

public:
	virtual void threadRecvProcFunc(char *pMsgBuf);
};

#endif