#include <stdio.h>
#include <motor/allocator.h>
#include <motor/arena.h>
#include <motor/file_watcher.h>

int main() {
    MtAllocator alloc;
    mt_arena_init(&alloc, 1 << 16);

    MtFileWatcher *watcher = mt_file_watcher_create(
        &alloc, MT_FILE_WATCHER_EVENT_MODIFY, "../assets");

    while (1) {
        MtFileWatcherEvent e;
        while (mt_file_watcher_poll(watcher, &e)) {
            switch (e.type) {
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
            if (e.src) printf("Source: %s\n", e.src);
            if (e.dst) printf("Destination: %s\n", e.dst);
            puts("==============");
        }
    }

    mt_file_watcher_destroy(watcher);

    mt_arena_destroy(&alloc);
    return 0;
}
