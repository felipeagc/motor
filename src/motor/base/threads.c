#include <motor/base/threads.h>

#include <stdlib.h>
#include <stdint.h>

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

    int res = fun(arg);

#if defined(MT_THREADS_WIN32)
    return (DWORD)res;
#else
    return (void *)(intptr_t)res;
#endif
}

void mt_thread_create(MtThread *thread, MtThreadStart func, void *arg)
{
    ThreadStartInfo *start_info = malloc(sizeof(ThreadStartInfo));
    *start_info                 = (ThreadStartInfo){
        .func = func,
        .arg  = arg,
    };

#if defined(MT_THREADS_WIN32)
    *thread = CreateThread(NULL, 0, thread_wrapper_function, (LPVOID)start_info, 0, NULL);
#elif defined(MT_THREADS_POSIX)
    pthread_create(thread, NULL, thread_wrapper_function, start_info);
#endif
}

MtThread mt_thread_current(void)
{
#if defined(MT_THREADS_WIN32)
    return GetCurrentThread();
#elif defined(MT_THREADS_POSIX)
    return pthread_self();
#endif
}

int mt_thread_detach(MtThread thread)
{
#if defined(MT_THREADS_WIN32)
    return CloseHandle(thread) != 0;
#elif defined(MT_THREADS_POSIX)
    return pthread_detach(thread) == 0;
#endif
}

int mt_thread_equal(MtThread thread1, MtThread thread2)
{
#if defined(MT_THREADS_WIN32)
    return GetThreadId(thread1) == GetThreadId(thread2);
#elif defined(MT_THREADS_POSIX)
    return pthread_equal(thread1, thread2);
#endif
}

void mt_thread_exit(int res)
{
#if defined(MT_THREADS_WIN32)
    ExitThread((DWORD)res);
#elif defined(MT_THREADS_POSIX)
    pthread_exit((void *)(intptr_t)res);
#endif
}

int mt_thread_join(MtThread thread, int *res)
{
#if defined(MT_THREADS_WIN32)
    DWORD dw_res;

    if (WaitForSingleObject(thread, INFINITE) == WAIT_FAILED)
    {
        return 0;
    }
    if (res != NULL)
    {
        if (GetExitCodeThread(thread, &dw_res) != 0)
        {
            *res = (int)dw_res;
        }
        else
        {
            return 0;
        }
    }
    CloseHandle(thread);
#elif defined(MT_THREADS_POSIX)
    void *pres;
    if (pthread_join(thread, &pres) != 0)
    {
        return 0;
    }
    if (res != NULL)
    {
        *res = (int)(intptr_t)pres;
    }
#endif
    return 1;
}
