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

MT_BASE_API uint32_t mt_cpu_count(void);

typedef struct MtThread
{
    uint64_t opaque;
} MtThread;

typedef int32_t (*MtThreadStart)(void *);

MT_BASE_API int32_t mt_thread_init(MtThread *thread, MtThreadStart func, void *arg);

MT_BASE_API MtThread mt_thread_current(void);

MT_BASE_API void mt_thread_sleep(uint32_t milliseconds);

MT_BASE_API int32_t mt_thread_detach(MtThread thread);

MT_BASE_API bool mt_thread_equal(MtThread thread1, MtThread thread2);

MT_BASE_API void mt_thread_exit(int32_t res);

MT_BASE_API int32_t mt_thread_wait(MtThread thread, int32_t *res);

typedef struct MtMutex
{
    union {
        void *align;
        uint8_t data[64];
    };
} MtMutex;

MT_BASE_API int32_t mt_mutex_init(MtMutex *mtx);

MT_BASE_API void mt_mutex_destroy(MtMutex *mtx);

MT_BASE_API int32_t mt_mutex_lock(MtMutex *mtx);

MT_BASE_API int32_t mt_mutex_trylock(MtMutex *mtx);

MT_BASE_API int32_t mt_mutex_unlock(MtMutex *mtx);

typedef struct MtCond
{
    union {
        void *align;
        uint8_t data[64];
    };
} MtCond;

MT_BASE_API int32_t mt_cond_init(MtCond *cond);

MT_BASE_API void mt_cond_destroy(MtCond *cond);

MT_BASE_API int32_t mt_cond_wake_one(MtCond *cond);

MT_BASE_API int32_t mt_cond_wake_all(MtCond *cond);

MT_BASE_API int32_t mt_cond_wait(MtCond *cond, MtMutex *mtx);
