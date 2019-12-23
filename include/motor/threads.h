#ifndef MT_THREADS_H
#define MT_THREADS_H

#if defined(_WIN32) || defined(__WIN32__) || defined(__WINDOWS__)
#define MT_THREADS_WIN32
#else
#define MT_THREADS_POSIX
#endif

#if defined(MT_THREADS_POSIX)
#include <pthread.h>
#elif defined(MT_THREADS_WIN32)
#include <windows.h>
#endif

#if !(defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201102L)) &&           \
    !defined(_Thread_local)
#if defined(__GNUC__) || defined(__INTEL_COMPILER) || defined(__SUNPRO_CC) ||  \
    defined(__IBMCPP__)
#define _Thread_local __thread
#else
#define _Thread_local __declspec(thread)
#endif
#elif defined(__GNUC__) && defined(__GNUC_MINOR__) &&                          \
    (((__GNUC__ << 8) | __GNUC_MINOR__) < ((4 << 8) | 9))
#define _Thread_local __thread
#endif

#define MT_THREAD_LOCAL _Thread_local

#if defined(MT_THREADS_POSIX)
typedef pthread_t MtThread;
#elif defined(MT_THREADS_WIN32)
typedef HANDLE MtThread;
#endif

typedef int (*MtThreadStart)(void *);

void mt_thread_create(MtThread *thread, MtThreadStart func, void *arg);

MtThread mt_thread_current(void);

int mt_thread_detach(MtThread thread);

int mt_thread_equal(MtThread thread1, MtThread thread2);

void mt_thread_exit(int res);

int mt_thread_join(MtThread thread, int *res);

#endif
