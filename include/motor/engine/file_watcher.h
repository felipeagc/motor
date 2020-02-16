#pragma once

#include "api_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct MtAllocator MtAllocator;
typedef struct MtFileWatcher MtFileWatcher;

typedef enum MtFileWatcherEventType {
    MT_FILE_WATCHER_EVENT_CREATE = (1 << 1), // file in "src" was just created.
    MT_FILE_WATCHER_EVENT_REMOVE = (1 << 2), // file in "src" was just removed.
    MT_FILE_WATCHER_EVENT_MODIFY = (1 << 3), // file in "src" was just modified.
    MT_FILE_WATCHER_EVENT_MOVE = (1 << 4),   /* file was moved from "src" to
    "dst", if "src" or "dst" is 0x0 it indicates that the path was outside the
    current watch. */

    MT_FILE_WATCHER_EVENT_ALL = MT_FILE_WATCHER_EVENT_CREATE | MT_FILE_WATCHER_EVENT_REMOVE |
                                MT_FILE_WATCHER_EVENT_MODIFY | MT_FILE_WATCHER_EVENT_MOVE,

    MT_FILE_WATCHER_EVENT_BUFFER_OVERFLOW // doc me
} MtFileWatcherEventType;

typedef struct MtFileWatcherEvent
{
    MtFileWatcherEventType type;
    char *src;
    char *dst;
} MtFileWatcherEvent;

typedef void (*MtFileWatcherHandler)(MtFileWatcherEvent *event, void *user_data);

MT_ENGINE_API MtFileWatcher *mt_file_watcher_create(
    MtAllocator *alloc,
    MtFileWatcherEventType types,
    MtFileWatcherHandler handler,
    const char *dir);

MT_ENGINE_API void mt_file_watcher_poll(MtFileWatcher *watcher, void *user_data);

MT_ENGINE_API void mt_file_watcher_destroy(MtFileWatcher *watcher);

#ifdef __cpluspus
}
#endif
