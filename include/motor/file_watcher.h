#pragma once

#include <stdbool.h>

typedef struct MtArena MtArena;
typedef struct MtFileWatcher MtFileWatcher;

typedef enum MtFileWatcherEventType {
    MT_FILE_WATCHER_EVENT_CREATE = (1 << 1), // file in "src" was just created.
    MT_FILE_WATCHER_EVENT_REMOVE = (1 << 2), // file in "src" was just removed.
    MT_FILE_WATCHER_EVENT_MODIFY = (1 << 3), // file in "src" was just modified.
    MT_FILE_WATCHER_EVENT_MOVE   = (1 << 4), /* file was moved from "src" to
    "dst", if "src" or "dst" is 0x0 it indicates that the path was outside the
    current watch. */

    MT_FILE_WATCHER_EVENT_ALL =
        MT_FILE_WATCHER_EVENT_CREATE | MT_FILE_WATCHER_EVENT_REMOVE |
        MT_FILE_WATCHER_EVENT_MODIFY | MT_FILE_WATCHER_EVENT_MOVE,

    MT_FILE_WATCHER_EVENT_BUFFER_OVERFLOW // doc me
} MtFileWatcherEventType;

typedef struct MtFileWatcherEvent {
    MtFileWatcherEventType type;
    char *src;
    char *dst;
} MtFileWatcherEvent;

MtFileWatcher *mt_file_watcher_create(
    MtArena *arena, MtFileWatcherEventType types, const char *dir);

bool mt_file_watcher_poll(MtFileWatcher *watcher, MtFileWatcherEvent *event);

void mt_file_watcher_destroy(MtFileWatcher *watcher);
