#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h> 
#include <errno.h>    
#include <unistd.h>
#include <algorithm>
#include "ngx_c_func.h"
#include "ngx_macro.h"
#include "ngx_c_conf.h"
#include "blocking_queue.h"
#include "ngx_global.h"
#include "ngx_c_socket.h"
#include "ngx_c_threadpool.h"
#include <thread>
#include <time.h>
#include <pthread.h>
//添加static后，除了该cpp中的函数可以调用以下四个函数外，其他都不可调用
static void ngx_start_worker_processes(int threadnums);
static int ngx_spawn_process(int threadnums,const char *pprocname);
static void ngx_worker_process_cycle(int inum,const char *pprocname);
static void ngx_worker_process_init(int inum);


void ngx_master_process_cycle(){
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGCHLD);     //子进程状态改变
    sigaddset(&set, SIGALRM);     //定时器超时
    sigaddset(&set, SIGIO);       //异步I/O
    sigaddset(&set, SIGINT);      //终端中断符
    sigaddset(&set, SIGHUP);      //连接断开
    sigaddset(&set, SIGUSR1);     //用户定义信号
    sigaddset(&set, SIGUSR2);     //用户定义信号
    sigaddset(&set, SIGWINCH);    //终端窗口大小改变
    sigaddset(&set, SIGTERM);     //终止
    sigaddset(&set, SIGQUIT);     //终端退出符
    //下方开始设置阻塞信号
    if(sigprocmask(SIG_BLOCK, &set, NULL) == -1){
        ngx_log_error_core(NGX_LOG_ALERT,errno,"ngx_master_process_cycle()中sigprocmask()失败!");
    }
    CConfig::CC_Ptr p_config = CConfig::get_instance();
    int wordprocess = p_config->GetInt("WorkerProcesses");
    if(wordprocess == -1){
        wordprocess = 1;
    }
    ngx_start_worker_processes(wordprocess);
    std::thread t1(print_log);
    std::thread t2(print_log);
    sigemptyset(&set);
    // int flag = 0;
    // while(true){
    //     sleep(1);
    //     if(flag < 10)
    //     log_blocking_queue.put(ngx_log_error_blocking_queue_core_str(0,0,"haha--这是父进程，pid为%P",ngx_pid));
    //     if(flag < 10)  flag ++;
    //     if(flag == 10)break;
    // }
    while(true){
        sigsuspend(&set);
        sleep(1);
    }
}

static void ngx_start_worker_processes(int threadnums)
{
    int i;
    for (i = 0; i < threadnums; i++)  //master进程在走这个循环，来创建若干个子进程
    {
        ngx_spawn_process(i,"worker process");
    } //end for
    return;
}

static int ngx_spawn_process(int inum,const char *pprocname)
{
    pid_t  pid;
    pid = fork(); //fork()系统调用产生子进程
    if(pid == -1){
        ngx_log_error_core(NGX_LOG_ALERT,errno,"ngx_spawn_process()fork()产生子进程num=%d,procname=\"%s\"失败!",inum,pprocname);
        return -1;
    }else if(pid == 0){
        ngx_parent = ngx_pid;              //因为是子进程了，所有原来的pid变成了父pid
        ngx_pid = getpid();                //重新获取pid,即本子进程的pid
        ngx_worker_process_cycle(inum,pprocname);    //我希望所有worker子进程，在这个函数里不断循环着不出来，也就是说，子进程流程不往下边走;
    }//end switch
    return pid;
}

static void ngx_worker_process_cycle(int inum,const char *pprocname) 
{
    //重新为子进程设置进程名，不要与父进程重复------
    ngx_worker_process_init(inum);
    ngx_setproctitle(pprocname); //设置标题   
    //暂时先放个死循环，我们在这个循环里一直不出来
    //setvbuf(stdout,NULL,_IONBF,0); //这个函数. 直接将printf缓冲区禁止， printf就直接输出了。
    int p_num = 0;
    for(;;)
    {

        //先sleep一下 以后扩充.......
        //printf("worker进程休息1秒");       
        //fflush(stdout); //刷新标准输出缓冲区，把输出缓冲区里的东西打印到标准输出设备上，则printf里的东西会立即输出；
        //sleep(1); //休息1秒       
        //usleep(100000);
        // if(p_num < 10)
        // log_blocking_queue.put(ngx_log_error_blocking_queue_core_str(0,0,"good--这是子进程，编号为%d,pid为%P!",inum,ngx_pid));
        // if(p_num < 10)
        // p_num ++;
        //printf("1212");
        //if(inum == 1)
        //{
            //ngx_log_stderr(0,"good--这是子进程，编号为%d,pid为%P",inum,ngx_pid); 
            //printf("good--这是子进程，编号为%d,pid为%d\r\n",inum,ngx_pid);
            //ngx_log_error_core(0,0,"good--这是子进程，编号为%d",inum,ngx_pid);
            //printf("我的测试哈inum=%d",inum++);
            //fflush(stdout);
        //}
        ngx_process_events_and_timers();
        //ngx_log_stderr(0,"good--这是子进程，pid为%P",ngx_pid); 
        //ngx_log_error_core(0,0,"good--这是子进程，编号为%d,pid为%P",inum,ngx_pid);

    } //end for(;;)
    g_threadpool.StopAll();
    g_socket.Shutdown_subproc();
    return;
}
//描述：子进程创建时调用本函数进行一些初始化工作
static void ngx_worker_process_init(int inum)
{
    sigset_t  set;      //信号集

    sigemptyset(&set);  //清空信号集
    if (sigprocmask(SIG_SETMASK, &set, NULL) == -1)  //原来是屏蔽那10个信号【防止fork()期间收到信号导致混乱】，现在不再屏蔽任何信号【接收任何信号】
    {
        ngx_log_error_core(NGX_LOG_ALERT,errno,"ngx_worker_process_init()中sigprocmask()失败!");
    }
    CConfig::CC_Ptr p_config = CConfig::get_instance();
    int tmpthreadnums = p_config->GetInt("ProcMsgRecvWorkThreadCount");
    if(tmpthreadnums == -1){
        tmpthreadnums = 4;
    }
    if(g_threadpool.Create(tmpthreadnums) == false){
        exit(-2);
    }
    sleep(1);
    if(g_socket.Initialize_subproc() == false){
        exit(-2);
    }
    g_socket.ngx_epoll_init();
    //....将来再扩充代码
    //....
    return;
}
