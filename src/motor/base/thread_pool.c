#include <motor/base/thread_pool.h>

#include <motor/base/array.h>
#include <motor/base/allocator.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

// TODO: handle queue resizing

MT_THREAD_LOCAL uint32_t mt_thread_pool_task_id = 0;

static inline uint32_t thread_pool_queue_size_no_lock(MtThreadPool *pool)
{

    if (pool->queue_front <= pool->queue_back)
    {
        return pool->queue_back - pool->queue_front;
    }
    else
    {
        return pool->queue_back + (mt_array_size(pool->queue) - pool->queue_front);
    }

    return 0;
}

static int32_t work(void *arg)
{
    MtThreadPoolWorker *worker = arg;
    MtThreadPool *pool         = worker->pool;
    mt_thread_pool_task_id     = worker->id;

    for (;;)
    {
        mt_mutex_lock(&pool->queue_mutex);
        while (!pool->stop && thread_pool_queue_size_no_lock(pool) == 0)
        {
            // Wait for new task or for shutdown
            mt_cond_wait(&pool->cond, &pool->queue_mutex);
        }

        if (pool->stop && thread_pool_queue_size_no_lock(pool) == 0)
        {
            mt_mutex_unlock(&pool->queue_mutex);
            return 0;
        }

        assert(thread_pool_queue_size_no_lock(pool) > 0);

        // Do work
        MtThreadPoolTask task = pool->queue[pool->queue_front];

        pool->queue_front = (pool->queue_front + 1) % mt_array_size(pool->queue);

        mt_mutex_unlock(&pool->queue_mutex);

        if (task.routine)
        {
            task.routine(task.arg);
        }

        mt_mutex_lock(&pool->queue_mutex);
        pool->num_working--;
        mt_cond_wake_all(&pool->done_cond);
        mt_mutex_unlock(&pool->queue_mutex);
    }

    return 0;
}

void mt_thread_pool_init(MtThreadPool *pool, uint32_t num_threads, MtAllocator *alloc)
{
    memset(pool, 0, sizeof(*pool));
    pool->alloc = alloc;

    mt_array_add(alloc, pool->queue, 128);

    mt_cond_init(&pool->cond);
    mt_cond_init(&pool->done_cond);
    mt_mutex_init(&pool->queue_mutex);

    mt_mutex_lock(&pool->queue_mutex);
    mt_mutex_unlock(&pool->queue_mutex);

    mt_array_add(alloc, pool->workers, num_threads);
    for (uint32_t i = 0; i < mt_array_size(pool->workers); ++i)
    {
        MtThreadPoolWorker *worker = &pool->workers[i];

        worker->pool = pool;
        worker->id   = i + 1;

        mt_thread_init(&worker->thread, work, worker);
    }
}

void mt_thread_pool_destroy(MtThreadPool *pool)
{
    mt_mutex_lock(&pool->queue_mutex);
    pool->stop = true;
    mt_cond_wake_all(&pool->cond);
    mt_mutex_unlock(&pool->queue_mutex);

    for (uint32_t i = 0; i < mt_array_size(pool->workers); ++i)
    {
        MtThreadPoolWorker *worker = &pool->workers[i];
        mt_thread_wait(worker->thread, NULL);
    }

    mt_array_free(pool->alloc, pool->workers);
    mt_array_free(pool->alloc, pool->queue);

    mt_mutex_destroy(&pool->queue_mutex);
    mt_cond_destroy(&pool->done_cond);
    mt_cond_destroy(&pool->cond);
}

void mt_thread_pool_enqueue(MtThreadPool *pool, MtThreadStart routine, void *arg)
{
    mt_mutex_lock(&pool->queue_mutex);
    pool->num_working++;
    pool->queue[pool->queue_back].routine = routine;
    pool->queue[pool->queue_back].arg     = arg;

    pool->queue_back = (pool->queue_back + 1) % mt_array_size(pool->queue);

    mt_cond_wake_one(&pool->cond);

    mt_mutex_unlock(&pool->queue_mutex);
}

bool mt_thread_pool_is_busy(MtThreadPool *pool)
{
    mt_mutex_lock(&pool->queue_mutex);
    bool busy = pool->num_working > 0;
    mt_mutex_unlock(&pool->queue_mutex);
    return busy;
}

void mt_thread_pool_wait_all(MtThreadPool *pool)
{
    mt_mutex_lock(&pool->queue_mutex);
    while (pool->num_working > 0)
    {
        // Wait for some task to be done
        mt_cond_wait(&pool->done_cond, &pool->queue_mutex);
    }

    mt_mutex_unlock(&pool->queue_mutex);
    return;
}

uint32_t mt_thread_pool_queue_size(MtThreadPool *pool)
{
    mt_mutex_lock(&pool->queue_mutex);
    uint32_t size = thread_pool_queue_size_no_lock(pool);
    mt_mutex_unlock(&pool->queue_mutex);
    return size;
}
