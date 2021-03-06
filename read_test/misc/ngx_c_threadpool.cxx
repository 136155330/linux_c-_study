#include <stdarg.h>
#include <unistd.h>

#include "ngx_global.h"
#include "ngx_c_func.h"
#include "ngx_c_threadpool.h"
#include "ngx_c_memory.h"
#include "ngx_macro.h"


//静态成员初始化
pthread_mutex_t CThreadPool::m_pthreadMutex = PTHREAD_MUTEX_INITIALIZER;  //#define PTHREAD_MUTEX_INITIALIZER ((pthread_mutex_t) -1)
pthread_cond_t CThreadPool::m_pthreadCond = PTHREAD_COND_INITIALIZER;     //#define PTHREAD_COND_INITIALIZER ((pthread_cond_t) -1)
bool CThreadPool::m_shutdown = false;    //刚开始标记整个线程池的线程是不退出的      

CThreadPool::CThreadPool(){
    m_iRunningThreadNum = 0;//正在运行的线程
    m_iLastEmgTime = 0; //就间隔时间写日志
    m_iRecvMsgQueueCount = 0;
}
CThreadPool::~CThreadPool(){
    clearMsgRecvQueue();
}
void CThreadPool::clearMsgRecvQueue(){
    char *p;
    CMemory *p_memory = CMemory::GetInstance();
    while(!m_MsgRecvQueue.empty()){
        p = m_MsgRecvQueue.front();
        m_MsgRecvQueue.pop_front();
        p_memory->FreeMemory(p);
    }
}
bool CThreadPool::Create(int threadNum){
    ThreadItem *pNew;
    int err;
    m_iThreadNum = threadNum;
    for(int i = 0; i < m_iThreadNum; i ++){
        m_threadVector.push_back(pNew = new ThreadItem(this));
        err = pthread_create(&pNew->_Handle, NULL, ThreadFunc, pNew);
        if(err != 0){
            ngx_log_stderr(err,"CThreadPool::Create()创建线程%d失败，返回的错误码为%d!",i,err);
            return false;
        }else{

        }
    }
    std::vector<ThreadItem*>::iterator iter;
lblfor:
    for(iter = m_threadVector.begin(); iter != m_threadVector.end(); iter ++){
        if((*iter) ->ifrunning == false){
            usleep(100 * 1000);
            goto lblfor;
        }
    }
    return true;
}

void * CThreadPool::ThreadFunc(void * threadData){
    ThreadItem *pThread = static_cast<ThreadItem *>(threadData);
    CThreadPool *pThreadPoolObj = pThread->_pThis;

    char *jobbuf = NULL;
    CMemory *p_memory = CMemory::GetInstance();
    int err;
    pthread_t tid = pthread_self();
    while(true){
        err = pthread_mutex_lock(&m_pthreadMutex);
        if(err != 0) ngx_log_stderr(err,"CThreadPool::ThreadFunc()pthread_mutex_lock()失败，返回的错误码为%d!",err);//有问题，要及时报告
        while(pThreadPoolObj->m_MsgRecvQueue.size() == 0 && m_shutdown == false){
            if(pThread->ifrunning == false){
                pThread->ifrunning = true;
            }
            pthread_cond_wait(&m_pthreadCond, &m_pthreadMutex);
        }
        if(m_shutdown){
            pthread_mutex_unlock(&m_pthreadMutex);
            break;//还mutex顺带把线程消亡
        }
        //下述还在临界区
        char *jobbuf = pThreadPoolObj->m_MsgRecvQueue.front();
        pThreadPoolObj->m_MsgRecvQueue.pop_front();
        --pThreadPoolObj->m_iRecvMsgQueueCount;
        err = pthread_mutex_unlock(&m_pthreadMutex); 
        if(err != 0)  ngx_log_stderr(err,"CThreadPool::ThreadFunc()pthread_cond_wait()失败，返回的错误码为%d!",err);//有问题，要及时报告
        //解除临界区
        ++ pThreadPoolObj->m_iRunningThreadNum;
        g_socket.threadRecvProcFunc(jobbuf); //个人感觉对应的锁释放应该放这里
        //因为没有共同使用的变量所以不存在问题
        p_memory->FreeMemory(jobbuf);
        --pThreadPoolObj->m_iRunningThreadNum;
    }
    return (void*)0;
}
void CThreadPool::StopAll(){
    if(m_shutdown)  return ;
    m_shutdown = true;
    int err = pthread_cond_broadcast(&m_pthreadCond);
    if(err != 0){
        ngx_log_stderr(err,"CThreadPool::StopAll()中pthread_cond_broadcast()失败，返回的错误码为%d!",err);
        return;
    }
    for(auto iter = m_threadVector.begin(); iter != m_threadVector.end(); ++ iter){
        pthread_join((*iter)->_Handle, NULL);
    }
    pthread_mutex_destroy(&m_pthreadMutex);
    pthread_cond_destroy(&m_pthreadCond);
    for(auto iter = m_threadVector.begin(); iter != m_threadVector.end(); ++ iter){
        if(*iter){
            delete *iter;
        }
    }
    m_threadVector.clear();
    ngx_log_stderr(0,"CThreadPool::StopAll()成功返回，线程池中线程全部正常结束!");
    return;  
}
void CThreadPool::inMsgRecvQueueAndSignal(char *buf){
    int err = pthread_mutex_lock(&m_pthreadMutex);     
    if(err != 0)
    {
        ngx_log_stderr(err,"CThreadPool::inMsgRecvQueueAndSignal()pthread_mutex_lock()失败，返回的错误码为%d!",err);
    }
    m_MsgRecvQueue.push_back(buf);
    err = pthread_mutex_unlock(&m_pthreadMutex);
    if(err != 0)
    {
        ngx_log_stderr(err,"CThreadPool::inMsgRecvQueueAndSignal()pthread_mutex_unlock()失败，返回的错误码为%d!",err);
    }
    Call();
    return ;
}
void CThreadPool::Call(){
    int err = pthread_cond_signal(&m_pthreadCond);
    if(err != 0 )
    {
        //这是有问题啊，要打印日志啊
        ngx_log_stderr(err,"CThreadPool::Call()中pthread_cond_signal()失败，返回的错误码为%d!",err);
    }
    if(m_iThreadNum == m_iRunningThreadNum){
        time_t currtime = time(NULL);
        if(currtime - m_iLastEmgTime > 10) //最少间隔10秒钟才报一次线程池中线程不够用的问题；
        {
            //两次报告之间的间隔必须超过10秒，不然如果一直出现当前工作线程全忙，但频繁报告日志也够烦的
            m_iLastEmgTime = currtime;  //更新时间
            //写日志，通知这种紧急情况给用户，用户要考虑增加线程池中线程数量了
            ngx_log_stderr(0,"CThreadPool::Call()中发现线程池中当前空闲线程数量为0，要考虑扩容线程池了!");
        }
    }
    return ;
}