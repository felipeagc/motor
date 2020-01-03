#include <stdio.h>
#include <motor/base/allocator.h>
#include <motor/base/arena.h>
#include <motor/engine/file_watcher.h>

void handler(MtFileWatcherEvent *e, void *user_data) {
    switch (e->type) {
    case MT_FILE_WATCHER_EVENT_CREATE: {
        printf("Created\n");
    } break;
    case MT_FILE_WATCHER_EVENT_MODIFY: {
        printf("Modified\n");
    } break;
    case MT_FILE_WATCHER_EVENT_MOVE: {
        printf("Moved\n");
    } break;
    case MT_FILE_WATCHER_EVENT_REMOVE: {
        printf("Removed\n");
    } break;
    default: break;
    }
    if (e->src) printf("Source: %s\n", e->src);
    if (e->dst) printf("Destination: %s\n", e->dst);
    puts("==============");
}

int main() {
    MtAllocator alloc;
    mt_arena_init(&alloc, 1 << 16);

    MtFileWatcher *watcher = mt_file_watcher_create(
        &alloc, MT_FILE_WATCHER_EVENT_MODIFY, "../assets");

    while (1) {
        mt_file_watcher_poll(watcher, handler, NULL);
    }

    mt_file_watcher_destroy(watcher);

    mt_arena_destroy(&alloc);
    return 0;
}
