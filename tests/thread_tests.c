#include <motor/base/threads.h>
#include <motor/base/thread_pool.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

int32_t thread_start(void *arg)
{
    printf("Hello from thread %u\n", mt_thread_pool_get_task_id());
    mt_thread_sleep(20);
    return 0;
}

int main()
{
    MtThreadPool pool;
    mt_thread_pool_init(&pool, 4, NULL);

    srand(0);

    for (uint32_t i = 0; i < 16; i++)
    {
        mt_thread_pool_enqueue(&pool, thread_start, NULL);
    }

    mt_thread_pool_wait_all(&pool);

    printf("Hello\n");

    for (uint32_t i = 0; i < 16; i++)
    {
        mt_thread_pool_enqueue(&pool, thread_start, NULL);
    }

    mt_thread_pool_destroy(&pool);

    return 0;
}
