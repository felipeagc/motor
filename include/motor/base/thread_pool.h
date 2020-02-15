#pragma once

#include "threads.h"

typedef struct MtAllocator MtAllocator;

typedef struct MtThreadPoolTask
{
    void *arg;
    MtThreadStart routine;
} MtThreadPoolTask;

typedef struct MtThreadPool MtThreadPool;

typedef struct MtThreadPoolWorker
{
    MtThreadPool *pool;
    MtThread thread;
    uint32_t id;
} MtThreadPoolWorker;

typedef struct MtThreadPool
{
    MtAllocator *alloc;
    /*array*/ MtThreadPoolWorker *workers;

    bool stop;

    uint32_t num_working;
    MtThreadPoolTask *queue;
    uint32_t queue_front;
    uint32_t queue_back;
    uint32_t queue_capacity;

    MtMutex queue_mutex;
    MtCond cond;
    MtCond done_cond;
} MtThreadPool;

MT_BASE_API uint32_t mt_thread_pool_get_task_id();

MT_BASE_API void mt_thread_pool_init(MtThreadPool *pool, uint32_t num_threads, MtAllocator *alloc);

MT_BASE_API void mt_thread_pool_destroy(MtThreadPool *pool);

MT_BASE_API void mt_thread_pool_enqueue(MtThreadPool *pool, MtThreadStart routine, void *arg);

MT_BASE_API bool mt_thread_pool_is_busy(MtThreadPool *pool);

MT_BASE_API void mt_thread_pool_wait_all(MtThreadPool *pool);

MT_BASE_API uint32_t mt_thread_pool_queue_size(MtThreadPool *pool);
