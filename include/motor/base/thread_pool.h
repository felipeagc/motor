#pragma once

#include "threads.h"

typedef struct MtAllocator MtAllocator;

extern MT_THREAD_LOCAL uint32_t mt_thread_pool_task_id;

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

    MtThreadPoolTask *queue;
    uint32_t queue_front;
    uint32_t queue_back;
    uint32_t queue_capacity;

    MtMutex queue_mutex;
    MtCond cond;
} MtThreadPool;

void mt_thread_pool_init(MtThreadPool *pool, uint32_t num_threads, MtAllocator *alloc);

void mt_thread_pool_destroy(MtThreadPool *pool);

void mt_thread_pool_enqueue(MtThreadPool *pool, MtThreadStart routine, void *arg);

uint32_t mt_thread_pool_queue_size(MtThreadPool *pool);
