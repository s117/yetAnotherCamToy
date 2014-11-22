#define THREAD_ENABLER_C
#include "thread_enabler.h"

int thread_enabler_init(thread_enabler_t *__enabler) {
    int rtnVal;
    rtnVal = pthread_cond_init(&__enabler->pcond, NULL);
    if (rtnVal == 0)
        return pthread_mutex_init(&__enabler->pmtx, NULL);
    else
        return rtnVal;
}

int thread_enabler_destory(thread_enabler_t *__enabler) {
    int rtnVal;
    rtnVal = pthread_cond_destroy(&__enabler->pcond);
    if (rtnVal == 0)
        return pthread_mutex_destroy(&__enabler->pmtx);
    else
        return rtnVal;
}

int thread_enabler_wait(thread_enabler_t *__enabler) {
    if (__enabler->isenable)
        return 0;
    else
        return pthread_cond_wait(&__enabler->pcond, &__enabler->pmtx);
}

int thread_enabler_enable(thread_enabler_t *__enabler) {
    if (__enabler->isenable)
        return 0;
    else {
        __enabler->isenable = 1;
        return pthread_cond_broadcast(&__enabler->pcond);
    }

}

int thread_enabler_disable(thread_enabler_t *__enabler) {
    __enabler->isenable = 0;
    return 0;
}
