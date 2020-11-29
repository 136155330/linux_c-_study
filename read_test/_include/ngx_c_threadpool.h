#ifndef __NGX_THREADPOOL_H__
#define __NGX_THREADPOOL_H__

#include <vector>
#include <pthread.h>
#include <atomic>

class CThreadPool{
public:
    CThreadPool();
    ~CThreadPool();
    bool Create(int threadNum);
    void StopAll();
    void Call(int irmqc);
private:
    static void * ThreadFunc(void *threadData);
    struct ThreadItem{
        pthread_t _Handle;
        CThreadPool *_pThis;
        bool ifrunning;
        ThreadItem(CThreadPool *pthis):_pThis(pthis), ifrunning(false){}
        ~ThreadItem(){}
    };
private:
    static pthread_mutex_t     m_pthreadMutex;      //线程同步互斥量/也叫线程同步锁
    static pthread_cond_t      m_pthreadCond;       //线程同步条件变量
    static bool                m_shutdown;          //线程退出标志，false不退出，true退出

    int                        m_iThreadNum;        //要创建的线程数量

    //int                        m_iRunningThreadNum; //线程数, 运行中的线程数	
    std::atomic<int>           m_iRunningThreadNum; //线程数, 运行中的线程数，原子操作
    time_t                     m_iLastEmgTime;      //上次发生线程不够用【紧急事件】的时间,防止日志报的太频繁
    //time_t                     m_iPrintInfoTime;    //打印信息的一个间隔时间，我准备10秒打印出一些信息供参考和调试
    //time_t                     m_iCurrTime;         //当前时间

    std::vector<ThreadItem *>  m_threadVector;   //线程 容器，容器里就是各个线程了  
};


#endif