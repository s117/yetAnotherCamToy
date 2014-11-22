#ifndef __H_P_THREAD_POOL
#define __H_P_THREAD_POOL
#include <pthread.h>
#include <sched.h>
#include <semaphore.h>
#include <stack>
#include <list>
#include "nullptr.h"


#define PTP_WAIT_TIME_INFINITE (unsigned int)-1
#define PTP_WAIT_TIME_IMMEDIATE 0x0

#define PTP_RETURN_OK 0x0
#define PTP_RETURN_RELEASE_NORMAL 0x1
#define PTP_RETURN_RELEASE_FORCE 0x2
#define PTP_RETURN_MEMOUT 0x3
#define PTP_RETURN_PTHREAD_LIB_FAIL 0x4
#define PTP_RETURN_ILLEGAL_STATE 0x5

class PThreadPool;

class TaskControlBlock {
public:
    PThreadPool *pool;
    sem_t jobsSem;
    pthread_rwlock_t jobsPS_rwlock;
    volatile int busyCnt;
    volatile int isRunning;
    std::stack<void*> jobsParamStack;
};

class PThreadPool {
public:
    PThreadPool(int maxThread);
    ~PThreadPool();
    virtual int InitThreadPool();
    virtual int ReleaseThreadPool(unsigned int waitTimes);
    virtual int KillAllThreadImme();
    virtual int QueryIdleThread();
    virtual int QueryWaitingJobs();
    virtual int DispatchJobs(void* args);
    virtual void innerAction(void* args) = 0;
protected:
    int numThread;
    pthread_t* threadList;
    TaskControlBlock tcb;
};

#endif // PTHREADPOOL_H
