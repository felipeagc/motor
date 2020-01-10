#include <motor/base/threads.h>

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#if defined(_WIN32) || defined(__WIN32__) || defined(__WINDOWS__)
#define MT_THREADS_WIN32
#else
#define MT_THREADS_POSIX
#endif

// clang-format off
#if defined(MT_THREADS_POSIX)
    #include <pthread.h>
    #include <unistd.h>

    static_assert(sizeof(MtThread) >= sizeof(pthread_t), "");
    static_assert(sizeof(MtMutex) >= sizeof(pthread_mutex_t), "");
    static_assert(sizeof(MtCond) >= sizeof(pthread_cond_t), "");
#elif defined(MT_THREADS_WIN32)
    #include <windows.h>

    typedef struct WindowsMutex
    {
        CRITICAL_SECTION cs;
    } WindowsMutex;

    typedef struct WindowCond
    {
        CONDITION_VARIABLE cv;
    } WindowsCond;

    static_assert(sizeof(MtThread) >= sizeof(HANDLE), "");
    static_assert(sizeof(MtMutex) >= sizeof(WindowsMutex), "");
    static_assert(sizeof(MtCond) >= sizeof(WindowsCond), "");
#endif
// clang-format on

uint32_t mt_cpu_count(void)
{
#if defined(MT_THREADS_WIN32)
    SYSTEM_INFO info;
    GetSystemInfo(&info);
    return (uint32_t)info.dwNumberOfProcessors;
#elif defined(MT_THREADS_POSIX)
    return (uint32_t)sysconf(_SC_NPROCESSORS_ONLN);
#endif
}

/*
 * Thread functions
 */

typedef struct
{
    MtThreadStart func;
    void *arg;
} ThreadStartInfo;

#if defined(MT_THREADS_WIN32)
static DWORD WINAPI thread_wrapper_function(LPVOID _arg)
#elif defined(MT_THREADS_POSIX)
static void *thread_wrapper_function(void *_arg)
#endif
{
    ThreadStartInfo *info = (ThreadStartInfo *)_arg;
    MtThreadStart fun     = info->func;
    void *arg             = info->arg;
    free(info);

    int32_t res = fun(arg);

#if defined(MT_THREADS_WIN32)
    return (DWORD)res;
#else
    return (void *)(intptr_t)res;
#endif
}

int32_t mt_thread_init(MtThread *thread, MtThreadStart func, void *arg)
{
    ThreadStartInfo *start_info = malloc(sizeof(ThreadStartInfo));
    *start_info                 = (ThreadStartInfo){
        .func = func,
        .arg  = arg,
    };

#if defined(MT_THREADS_WIN32)
    HANDLE _thread = CreateThread(NULL, 0, thread_wrapper_function, (LPVOID)start_info, 0, NULL);
    int32_t res    = (_thread != NULL);
    memcpy(thread, &_thread, sizeof(_thread));
    return res;
#elif defined(MT_THREADS_POSIX)
    pthread_t _thread;
    int32_t res = pthread_create(&_thread, NULL, thread_wrapper_function, start_info) == 0;
    memcpy(thread, &_thread, sizeof(_thread));
    return res;
#endif
}

MtThread mt_thread_current(void)
{
#if defined(MT_THREADS_WIN32)
    MtThread thread;
    HANDLE _thread = GetCurrentThread();
    memcpy(&thread, &_thread, sizeof(_thread));
    return thread;
#elif defined(MT_THREADS_POSIX)
    MtThread thread;
    pthread_t _thread = pthread_self();
    memcpy(&thread, &_thread, sizeof(_thread));
    return thread;
#endif
}

void mt_thread_sleep(uint32_t milliseconds)
{
#if defined(MT_THREADS_WIN32)
    Sleep((DWORD)milliseconds);
#elif defined(MT_THREADS_POSIX)
    usleep((useconds_t)(1000 * milliseconds));
#endif
}

int32_t mt_thread_detach(MtThread thread)
{
#if defined(MT_THREADS_WIN32)

    HANDLE _thread = *((HANDLE *)&thread);
    return CloseHandle(_thread) != 0;
#elif defined(MT_THREADS_POSIX)
    pthread_t _thread = *((pthread_t *)&thread);
    return pthread_detach(_thread) == 0;
#endif
}

bool mt_thread_equal(MtThread thread1, MtThread thread2)
{
#if defined(MT_THREADS_WIN32)
    HANDLE _thread1 = *((HANDLE *)&thread1);
    HANDLE _thread2 = *((HANDLE *)&thread2);
    return GetThreadId(_thread1) == GetThreadId(_thread2);
#elif defined(MT_THREADS_POSIX)
    pthread_t _thread1 = *((pthread_t *)&thread1);
    pthread_t _thread2 = *((pthread_t *)&thread2);
    return (bool)pthread_equal(_thread1, _thread2);
#endif
}

void mt_thread_exit(int32_t res)
{
#if defined(MT_THREADS_WIN32)
    ExitThread((DWORD)res);
#elif defined(MT_THREADS_POSIX)
    pthread_exit((void *)(intptr_t)res);
#endif
}

int32_t mt_thread_wait(MtThread thread, int32_t *res)
{
#if defined(MT_THREADS_WIN32)
    HANDLE _thread = *((HANDLE *)&thread);

    if (WaitForSingleObject(_thread, INFINITE) == WAIT_FAILED)
    {
        return 0;
    }
    if (res != NULL)
    {
        if (GetExitCodeThread(_thread, res) == 0)
        {
            return 0;
        }
    }

    CloseHandle(_thread.cs);

    return 1;
#elif defined(MT_THREADS_POSIX)
    pthread_t _thread = *((pthread_t *)&thread);

    void *pres;
    if (pthread_join(_thread, &pres) != 0)
    {
        return 0;
    }
    if (res != NULL)
    {
        *res = (int32_t)pres;
    }

    return 1;
#endif
}

/*
 * Mutex functions
 */

int32_t mt_mutex_init(MtMutex *mtx)
{
#if defined(MT_THREADS_WIN32)
    WindowsMutex *_mtx = (WindowsMutex *)mtx;
    InitializeCriticalSection(&(_mtx->cs));
    return 1;
#elif defined(MT_THREADS_POSIX)
    return pthread_mutex_init((pthread_mutex_t *)mtx, NULL) == 0;
#endif
}

void mt_mutex_destroy(MtMutex *mtx)
{
#if defined(MT_THREADS_WIN32)
    WindowsMutex *_mtx = (WindowsMutex *)mtx;
    DeleteCriticalSection(&(_mtx->cs));
#elif defined(MT_THREADS_POSIX)
    pthread_mutex_destroy((pthread_mutex_t *)mtx);
#endif
}

int32_t mt_mutex_lock(MtMutex *mtx)
{
#if defined(MT_THREADS_WIN32)
    WindowsMutex *_mtx = (WindowsMutex *)mtx;
    EnterCriticalSection(&(_mtx->cs));
    return 1;
#elif defined(MT_THREADS_POSIX)
    return pthread_mutex_lock((pthread_mutex_t *)mtx) == 0;
#endif
}

int32_t mt_mutex_trylock(MtMutex *mtx)
{
#if defined(MT_THREADS_WIN32)
    WindowsMutex *_mtx = (WindowsMutex *)mtx;
    return TryEnterCriticalSection(&(_mtx->cs));
#elif defined(MT_THREADS_POSIX)
    return pthread_mutex_trylock((pthread_mutex_t *)mtx) == 0;
#endif
}

int32_t mt_mutex_unlock(MtMutex *mtx)
{
#if defined(MT_THREADS_WIN32)
    WindowsMutex *_mtx = (WindowsMutex *)mtx;
    LeaveCriticalSection(&(_mtx->cs));
    return 1;
#elif defined(MT_THREADS_POSIX)
    return pthread_mutex_unlock((pthread_mutex_t *)mtx) == 0;
#endif
}

/*
 * Cond functions
 */

int32_t mt_cond_init(MtCond *cond)
{
#if defined(MT_THREADS_WIN32)
    WindowsCond *_cond = (WindowsCond *)cond;
    InitializeConditionVariable(&_cond->cv);
    return 1;
#elif defined(MT_THREADS_POSIX)
    return pthread_cond_init((pthread_cond_t *)cond, NULL) == 0;
#endif
}

void mt_cond_destroy(MtCond *cond)
{
#if defined(MT_THREADS_WIN32)
    // Nothing needed here for windows.
#elif defined(MT_THREADS_POSIX)
    pthread_cond_destroy((pthread_cond_t *)cond);
#endif
}

int32_t mt_cond_wake_one(MtCond *cond)
{
#if defined(MT_THREADS_WIN32)
    WindowsCond *_cond = (WindowsCond *)cond;
    WakeConditionVariable(&_cond->cv);
    return 1;
#elif defined(MT_THREADS_POSIX)
    return pthread_cond_signal((pthread_cond_t *)cond) == 0;
#endif
}

int32_t mt_cond_wake_all(MtCond *cond)
{
#if defined(MT_THREADS_WIN32)
    WindowsCond *_cond = (WindowsCond *)cond;
    WakeAllConditionVariable(&_cond->cv);
    return 1;
#elif defined(MT_THREADS_POSIX)
    return pthread_cond_broadcast((pthread_cond_t *)cond) == 0;
#endif
}

int32_t mt_cond_wait(MtCond *cond, MtMutex *mtx)
{
#if defined(MT_THREADS_WIN32)
    WindowsCond *_cond = (WindowsCond *)cond;
    WindowsMutex *_mtx = (WindowsMutex *)mtx;
    return !!SleepConditionVariableCS(&_cond->cv, &_mtx->cs, INFINITE);
#elif defined(MT_THREADS_POSIX)
    return pthread_cond_wait((pthread_cond_t *)cond, (pthread_mutex_t *)mtx) == 0;
#endif
}
