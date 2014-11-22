#ifndef THREAD_ENABLER_H
#define THREAD_ENABLER_H
#include "pthread.h"

#ifdef THREAD_ENABLER_C
#define EXTERN
#else
#define EXTERN extern
#endif

#define THREAD_ENABLER_T_INITIALIZER {false,PTHREAD_COND_INITIALIZER,PTHREAD_MUTEX_INITIALIZER}

#ifdef __cplusplus
extern "C" {
#endif

typedef struct __thread_enabler_t {
    volatile int isenable;
    pthread_cond_t pcond;
    pthread_mutex_t pmtx;
} thread_enabler_t;

EXTERN int thread_enabler_init(thread_enabler_t *__enabler);
EXTERN int thread_enabler_destory(thread_enabler_t *__enabler);
EXTERN int thread_enabler_wait(thread_enabler_t *__enabler);
EXTERN int thread_enabler_enable(thread_enabler_t *__enabler);
EXTERN int thread_enabler_disable(thread_enabler_t *__enabler);

#ifdef __cplusplus
}
#endif


#endif // THREAD_ENABLER_H
