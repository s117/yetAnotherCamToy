#define __C_P_THREAD_POOL
#include "PThreadPool.h"

#include <stdlib.h>
#include <unistd.h>
//class PThreadPool{
//public:
//    PThreadPool(int maxThread);
//    virtual int InitThreadPool();
//    virtual int ReleaseThreadPool();
//    virtual int QueryThreadPool();
//    virtual int DispatchJobs();
//    virtual void* innerAction(void* args) = 0;
//protected:
//    int numThread;
//    pthread_t listenThread;
//    std::stack<pthread_t> readyThPool;
//    std::list<pthread_t> thList;
//};

//trampoline function to get argument and invoke user's logic

static void* trampoline(void *args) {
    TaskControlBlock *tcb = (TaskControlBlock*) args;
    while (tcb->isRunning) {
        sem_wait(&tcb->jobsSem);
        if (!tcb->isRunning) // get EXIT message
            break;
        pthread_rwlock_wrlock(&tcb->jobsPS_rwlock);
        void* innerArgs = tcb->jobsParamStack.top();
        tcb->jobsParamStack.pop();
        ++tcb->busyCnt;
        pthread_rwlock_unlock(&tcb->jobsPS_rwlock);

        tcb->pool->innerAction(innerArgs);

        pthread_rwlock_wrlock(&tcb->jobsPS_rwlock);
        --tcb->busyCnt;
        pthread_rwlock_unlock(&tcb->jobsPS_rwlock);
    }
    return (void*) PTP_RETURN_OK;
}

PThreadPool::PThreadPool(int maxThread) {
    numThread = maxThread;
    tcb.busyCnt = 0;
    tcb.isRunning = false;
    threadList = nullptr;
    tcb.pool = this;

}

PThreadPool::~PThreadPool() {
    KillAllThreadImme();
}

//allocate resource and start thread pool

int PThreadPool::InitThreadPool() {
    if (!tcb.isRunning) {
        if ((threadList = (pthread_t*) malloc(sizeof (pthread_t) * numThread)) == nullptr)
            return PTP_RETURN_MEMOUT;
        tcb.isRunning = true;
        if (sem_init(&tcb.jobsSem, 0, 0))
            return PTP_RETURN_PTHREAD_LIB_FAIL;
        if (pthread_rwlock_init(&tcb.jobsPS_rwlock, NULL))
            return PTP_RETURN_PTHREAD_LIB_FAIL;
        tcb.busyCnt = 0;
        while (!tcb.jobsParamStack.empty())
            tcb.jobsParamStack.pop();
        for (int i = 0; i < numThread; ++i) {
            pthread_create(threadList + i, NULL, trampoline, &tcb);
        }
        return PTP_RETURN_OK;
    } else {
        return PTP_RETURN_ILLEGAL_STATE;
    }

}

//wait all running thread finish work, and free all res takes

int PThreadPool::ReleaseThreadPool(unsigned int waitTimes) {
    unsigned int tryCnt = 0;
    if (tcb.isRunning) {
        tcb.isRunning = false;
        while (threadList != nullptr) {
            if (waitTimes != PTP_WAIT_TIME_INFINITE)
                if (tryCnt >= waitTimes) {
                    KillAllThreadImme();
                    return PTP_RETURN_RELEASE_FORCE;
                }
            pthread_rwlock_wrlock(&tcb.jobsPS_rwlock);
            if (tcb.busyCnt == 0) {
                KillAllThreadImme();
                return PTP_RETURN_RELEASE_NORMAL;
            }
            pthread_rwlock_unlock(&tcb.jobsPS_rwlock);
            usleep(200);
            ++tryCnt;
        }
        return PTP_RETURN_RELEASE_NORMAL;
    } else {
        return PTP_RETURN_ILLEGAL_STATE;
    }
}

//force kill all thread and free res allocate by PThreadPool immediate. Please note invoke this method may cause the resource used by working thread have no chance to release

int PThreadPool::KillAllThreadImme() {
    if (threadList != nullptr) {
        tcb.isRunning = false;
        for (int i = 0; i < numThread; ++i) {
            sem_post(&tcb.jobsSem);
        }
        for (int i = 0; i < numThread; ++i) {
            void* rtnVal;
            pthread_cancel(threadList[i]);
            pthread_join(threadList[i], &rtnVal);
        }
        free(threadList);
        tcb.busyCnt = 0;
        tcb.isRunning = false;
        threadList = nullptr;
        pthread_rwlock_unlock(&tcb.jobsPS_rwlock); //release rwlock to destory
        pthread_rwlock_destroy(&tcb.jobsPS_rwlock);
        sem_destroy(&tcb.jobsSem); //no thread blocked on sem now

        while (!tcb.jobsParamStack.empty())
            tcb.jobsParamStack.pop();
    }
    return PTP_RETURN_RELEASE_FORCE;
}

//return how many thread are free

int PThreadPool::QueryIdleThread() {
    if (tcb.isRunning)
        return numThread - tcb.busyCnt;
    else
        return 0;
}

int PThreadPool::QueryWaitingJobs() {
    if (tcb.isRunning) {
        int waitingNum = 0;
        return sem_getvalue(&tcb.jobsSem, &waitingNum);
    } else
        return 0;
}

//dipatch a jobs

int PThreadPool::DispatchJobs(void* args) {
    if (pthread_rwlock_wrlock(&tcb.jobsPS_rwlock))
        return PTP_RETURN_PTHREAD_LIB_FAIL;
    tcb.jobsParamStack.push(args);
    if (pthread_rwlock_unlock(&tcb.jobsPS_rwlock))
        return PTP_RETURN_PTHREAD_LIB_FAIL;
    if (sem_post(&tcb.jobsSem))
        return PTP_RETURN_PTHREAD_LIB_FAIL;
    return PTP_RETURN_OK;
}
