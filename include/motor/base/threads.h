#pragma once

#include "api_types.h"

#if !(defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201102L)) && !defined(_Thread_local)
#if defined(__GNUC__) || defined(__INTEL_COMPILER) || defined(__SUNPRO_CC) || defined(__IBMCPP__)
#define _Thread_local __thread
#else
#define _Thread_local __declspec(thread)
#endif
#elif defined(__GNUC__) && defined(__GNUC_MINOR__) &&                                              \
    (((__GNUC__ << 8) | __GNUC_MINOR__) < ((4 << 8) | 9))
#define _Thread_local __thread
#endif

#define MT_THREAD_LOCAL _Thread_local

uint32_t mt_cpu_count(void);

typedef struct MtThread
{
    uint64_t opaque;
} MtThread;

typedef int32_t (*MtThreadStart)(void *);

int32_t mt_thread_init(MtThread *thread, MtThreadStart func, void *arg);

MtThread mt_thread_current(void);

void mt_thread_sleep(uint32_t milliseconds);

int32_t mt_thread_detach(MtThread thread);

bool mt_thread_equal(MtThread thread1, MtThread thread2);

void mt_thread_exit(int32_t res);

int32_t mt_thread_wait(MtThread thread, int32_t *res);

typedef struct MtMutex
{
    union {
        void *align;
        uint8_t data[64];
    };
} MtMutex;

int32_t mt_mutex_init(MtMutex *mtx);

void mt_mutex_destroy(MtMutex *mtx);

int32_t mt_mutex_lock(MtMutex *mtx);

int32_t mt_mutex_trylock(MtMutex *mtx);

int32_t mt_mutex_unlock(MtMutex *mtx);

typedef struct MtCond
{
    union {
        void *align;
        uint8_t data[64];
    };
} MtCond;

int32_t mt_cond_init(MtCond *cond);

void mt_cond_destroy(MtCond *cond);

int32_t mt_cond_wake_one(MtCond *cond);

int32_t mt_cond_wake_all(MtCond *cond);

int32_t mt_cond_wait(MtCond *cond, MtMutex *mtx);
