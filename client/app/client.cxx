#include "ngx_comm.h"
#include <iostream>
#include <cstdio>
#include <cstring>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <cstdlib>
#include <thread>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/ioctl.h> //ioctl
#include <arpa/inet.h>
#include <fcntl.h>     //open
#include <errno.h>     //errno
#include <sys/socket.h>
#include <sys/ioctl.h> //ioctl
#include <arpa/inet.h>
#include "ngx_c_crc32.h"
int Connect(){
    int sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); //连接sockfd
    struct sockaddr_in in_addr;
    memset(&in_addr, 0, sizeof(in_addr));
    in_addr.sin_family = AF_INET;
    in_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    in_addr.sin_port = htons(80);
    printf("........\n");
    int f = connect(sockfd, (struct sockaddr *)&in_addr, sizeof(struct sockaddr_in));
    printf("??????????\n");
    printf("sockfd = %d\n", sockfd);    
    if(f == -1){
        printf("connect函数调用失败\n");
        return f;
    }
    printf("connect函数已经调用\n");
    return sockfd;
}
int sendData(int fd, char *p, int p_len){
    int usend = p_len;
    int wrote = 0;
    int temp_sret;
    while(wrote < usend){
        temp_sret = send(fd, p + wrote, usend - wrote, 0);
        if (temp_sret == 0)
		{
			//有错误发生了
			return -1;
		}
		wrote += temp_sret;
    }
    return wrote;
}
int main(){
    int flag, sk_fd;
    sk_fd = -1;
    while(true){
        printf("the start\n");
        printf("1. connect\n");
        printf("2. senddata\n");
        scanf("%d", &flag);
        if(flag == 1){
            printf("flag == 1\n");
            sk_fd = Connect();
        }
        if(flag == 2){
            char *p = (char *)new char[sizeof(COMM_PKG_HEADER) + sizeof(STRUCT_REGISTER)];

            LPCOMM_PKG_HEADER pinfohead;
            pinfohead = (LPCOMM_PKG_HEADER)p;
            pinfohead->msgCode = 5;
            pinfohead->msgCode = htons(pinfohead->msgCode);
            pinfohead->crc32 = CCRC32::GetInstance()->Get_CRC((unsigned char *)(p + sizeof(COMM_PKG_HEADER)),  sizeof(STRUCT_REGISTER));
            pinfohead->crc32 = htonl(pinfohead->crc32);
            pinfohead->pkgLen = htons(sizeof(COMM_PKG_HEADER) + sizeof(STRUCT_REGISTER));
            //int calccrc = CCRC32::GetInstance()->Get_CRC((unsigned char *)pPkgBody,pkglen-m_iLenPkgHeader);
            //LPSTRUCT_LOGIN p_2 = (LPSTRUCT_LOGIN)(p + sizeof(COMM_PKG_HEADER));
            //strcpy(p_2->username, "5678");
            //strcpy(p_2->password, "123456");
            if(sendData(sk_fd, p, sizeof(COMM_PKG_HEADER) + sizeof(STRUCT_REGISTER)) == -1){
                printf("发生错误了\n");
                return 1;
            }
            delete [] p;
        }
    }
}