#include <motor/engine/file_watcher.h>

#include <motor/base/allocator.h>
#include <motor/base/array.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>

#if defined(__linux__)
#include <unistd.h>
#include <sys/inotify.h>
#include <dirent.h>
#include <sys/stat.h>

typedef struct WatcherItem {
    int wd;
    char *path;
} WatcherItem;

struct MtFileWatcher {
    MtAllocator *alloc;
    int notifierfd;

    uint32_t watch_flags;

    /*array*/ WatcherItem *items;
};

static void watcher_add(MtFileWatcher *w, char *path) {
    int wd = inotify_add_watch(w->notifierfd, path, w->watch_flags);
    if (wd < 0) {
        return;
    }
    WatcherItem item = {.wd = wd, .path = mt_alloc(w->alloc, strlen(path) + 1)};
    strncpy(item.path, path, strlen(path) + 1);
    mt_array_push(w->alloc, w->items, item);
}
static void watcher_remove(MtFileWatcher *w, int wd) {
    for (size_t i = 0; i < mt_array_size(w->items); ++i) {
        if (wd != w->items[i].wd) continue;

        mt_free(w->alloc, w->items[i].path);
        w->items[i].wd   = 0;
        w->items[i].path = 0x0;

        size_t swap_index = mt_array_size(w->items) - 1;
        if (i != swap_index)
            memcpy(w->items + i, w->items + swap_index, sizeof(WatcherItem));
        mt_array_set_size(w->items, mt_array_size(w->items) - 1);
        return;
    }
}

static void watcher_recursive_add(
    MtFileWatcher *w, char *path_buffer, size_t path_len, size_t path_max) {
    watcher_add(w, path_buffer);
    DIR *dirp = opendir(path_buffer);
    struct dirent *ent;
    while ((ent = readdir(dirp)) != 0x0) {
        if ((ent->d_type != DT_DIR && ent->d_type != DT_LNK &&
             ent->d_type != DT_UNKNOWN))
            continue;

        if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0)
            continue;

        size_t d_name_size = strlen(ent->d_name);
        assert(path_len + d_name_size + 2 < path_max);

        strcpy(path_buffer + path_len, ent->d_name);
        path_buffer[path_len + d_name_size]     = '/';
        path_buffer[path_len + d_name_size + 1] = '\0';

        if (ent->d_type == DT_LNK || ent->d_type == DT_UNKNOWN) {
            struct stat statbuf;
            if (stat(path_buffer, &statbuf) == -1) continue;

            if (!S_ISDIR(statbuf.st_mode)) continue;
        }

        watcher_recursive_add(
            w, path_buffer, path_len + d_name_size + 1, path_max);
    }
    path_buffer[path_len] = '\0';

    closedir(dirp);
}

MtFileWatcher *mt_file_watcher_create(
    MtAllocator *alloc, MtFileWatcherEventType types, const char *dir) {
    MtFileWatcher *w = mt_alloc(alloc, sizeof(MtFileWatcher));
    memset(w, 0, sizeof(*w));

    w->alloc = alloc;

    if (types & MT_FILE_WATCHER_EVENT_CREATE) w->watch_flags |= IN_CREATE;
    if (types & MT_FILE_WATCHER_EVENT_REMOVE) w->watch_flags |= IN_DELETE;
    if (types & MT_FILE_WATCHER_EVENT_MODIFY) w->watch_flags |= IN_MODIFY;
    if (types & MT_FILE_WATCHER_EVENT_MOVE) w->watch_flags |= IN_MOVE;

    w->watch_flags |= IN_DELETE_SELF;

    w->notifierfd = inotify_init1(IN_NONBLOCK);
    if (w->notifierfd == -1) {
        mt_free(alloc, w);
        return NULL;
    }

    char path_buffer[4096];
    strncpy(path_buffer, dir, sizeof(path_buffer));

    size_t path_len = strlen(path_buffer);
    if (path_buffer[path_len - 1] != '/') {
        path_buffer[path_len]     = '/';
        path_buffer[path_len + 1] = '\0';
        ++path_len;
    }

    watcher_recursive_add(w, path_buffer, path_len, sizeof(path_buffer));

    return w;
}

static const char *find_wd_path(MtFileWatcher *w, int wd) {
    for (size_t i = 0; i < mt_array_size(w->items); ++i)
        if (wd == w->items[i].wd) return w->items[i].path;
    return 0x0;
}

static char *build_full_path(
    MtFileWatcher *watcher, int wd, const char *name, uint32_t name_len) {
    const char *dirpath = find_wd_path(watcher, wd);
    size_t dirlen       = strlen(dirpath);
    size_t length       = dirlen + 1 + name_len;
    char *res           = mt_alloc(watcher->alloc, length + 1);
    if (res) {
        memcpy(res, dirpath, dirlen);
        memcpy(res + dirlen, name, name_len);
        res[length - 1] = 0;
    }
    return res;
}

void mt_file_watcher_poll(
    MtFileWatcher *w, MtFileWatcherHandler handler, void *user_data) {
    char *move_src       = 0x0;
    uint32_t move_cookie = 0;

    char read_buffer[4096];
    ssize_t read_bytes = read(w->notifierfd, read_buffer, sizeof(read_buffer));
    if (read_bytes > 0) {
        for (char *bufp = read_buffer; bufp < read_buffer + read_bytes;) {
            struct inotify_event *ev = (struct inotify_event *)bufp;
            bool is_dir              = (ev->mask & IN_ISDIR);
            bool is_create           = (ev->mask & IN_CREATE);
            bool is_remove           = (ev->mask & IN_DELETE);
            bool is_modify           = (ev->mask & IN_MODIFY);
            bool is_move_from        = (ev->mask & IN_MOVED_FROM);
            bool is_move_to          = (ev->mask & IN_MOVED_TO);
            bool is_del_self         = (ev->mask & IN_DELETE_SELF);

            if (is_dir) {
                if (is_create) {
                    char *src = build_full_path(w, ev->wd, ev->name, ev->len);
                    watcher_add(w, src);
                    MtFileWatcherEvent e = {
                        MT_FILE_WATCHER_EVENT_CREATE, src, NULL};
                    handler(&e, user_data);
                    mt_free(w->alloc, src);
                } else if (is_remove) {
                    char *src = build_full_path(w, ev->wd, ev->name, ev->len);
                    MtFileWatcherEvent e = {
                        MT_FILE_WATCHER_EVENT_REMOVE, src, NULL};
                    handler(&e, user_data);
                    mt_free(w->alloc, src);
                } else if (is_del_self) {
                    watcher_remove(w, ev->wd);
                }
            } else if (ev->mask & IN_Q_OVERFLOW) {
                MtFileWatcherEvent e = {
                    MT_FILE_WATCHER_EVENT_BUFFER_OVERFLOW, NULL, NULL};
                handler(&e, user_data);
            } else {
                if (is_create) {
                    char *src = build_full_path(w, ev->wd, ev->name, ev->len);
                    MtFileWatcherEvent e = {
                        MT_FILE_WATCHER_EVENT_CREATE, src, NULL};
                    handler(&e, user_data);
                    mt_free(w->alloc, src);
                } else if (is_remove) {
                    char *src = build_full_path(w, ev->wd, ev->name, ev->len);
                    MtFileWatcherEvent e = {
                        MT_FILE_WATCHER_EVENT_REMOVE, src, NULL};
                    handler(&e, user_data);
                    mt_free(w->alloc, src);
                } else if (is_modify) {
                    char *src = build_full_path(w, ev->wd, ev->name, ev->len);
                    MtFileWatcherEvent e = {
                        MT_FILE_WATCHER_EVENT_MODIFY, src, NULL};
                    handler(&e, user_data);
                    mt_free(w->alloc, src);
                } else if (is_move_from) {
                    if (move_src != 0x0) {
                        // ... this is a new pair of a move, so the last one was
                        // move "outside" the current watch ...
                        MtFileWatcherEvent e = {
                            MT_FILE_WATCHER_EVENT_MOVE, move_src, 0x0};
                        handler(&e, user_data);
                        mt_free(w->alloc, move_src);
                    }

                    // ... this is the first potential pair of a move ...
                    move_src    = build_full_path(w, ev->wd, ev->name, ev->len);
                    move_cookie = ev->cookie;
                } else if (is_move_to) {
                    if (move_src && move_cookie == ev->cookie) {
                        // ... this is the dst for a move ...
                        char *dst =
                            build_full_path(w, ev->wd, ev->name, ev->len);
                        MtFileWatcherEvent e = {
                            MT_FILE_WATCHER_EVENT_MOVE, move_src, dst};
                        handler(&e, user_data);
                        mt_free(w->alloc, dst);
                        mt_free(w->alloc, move_src);

                        move_src    = 0x0;
                        move_cookie = 0;
                    } else if (move_src != 0x0) {
                        // ... this is a "move to outside of watch" ...
                        MtFileWatcherEvent e = {
                            MT_FILE_WATCHER_EVENT_MOVE, move_src, 0x0};
                        handler(&e, user_data);
                        mt_free(w->alloc, move_src);

                        move_src    = 0x0;
                        move_cookie = 0;

                        // ...followed by a "move from outside to watch ...
                        char *dst =
                            build_full_path(w, ev->wd, ev->name, ev->len);
                        e = (MtFileWatcherEvent){
                            MT_FILE_WATCHER_EVENT_MOVE, NULL, dst};
                        handler(&e, user_data);
                        mt_free(w->alloc, dst);
                    } else {
                        // ... this is a "move from outside to watch" ...
                        char *dst =
                            build_full_path(w, ev->wd, ev->name, ev->len);
                        MtFileWatcherEvent e = {
                            MT_FILE_WATCHER_EVENT_MOVE, NULL, dst};
                        handler(&e, user_data);
                        mt_free(w->alloc, dst);
                    }
                }
            }

            bufp += sizeof(struct inotify_event) + ev->len;
        }

        if (move_src) {
            // ... we have a "move to outside of watch" that was never closed
            // ...
            MtFileWatcherEvent e = {MT_FILE_WATCHER_EVENT_MOVE, move_src, NULL};
            handler(&e, user_data);
            mt_free(w->alloc, move_src);
        }
    }
}

void mt_file_watcher_destroy(MtFileWatcher *w) {
    close(w->notifierfd);

    for (uint32_t i = 0; i < mt_array_size(w->items); i++) {
        mt_free(w->alloc, w->items[i].path);
    }

    mt_array_free(w->alloc, w->items);
    mt_free(w->alloc, w);
}
#endif

#if defined(_WIN32) || defined(__WIN32__) || defined(__WINDOWS__)
#include <windows.h>

struct MtFileWatcher {
    MtAllocator *alloc;

    const char *watch_dir;
    size_t watch_dir_len;

    HANDLE directory;
    OVERLAPPED overlapped;

    DWORD read_buffer[2048];
};

static void watcher_begin_read(MtFileWatcher *w) {
    ZeroMemory(&w->overlapped, sizeof(w->overlapped));

    BOOL success = ReadDirectoryChangesW(
        w->directory,
        w->read_buffer,
        sizeof(w->read_buffer),
        TRUE, // recursive
        FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_CREATION |
            FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME,
        0x0,
        &w->overlapped,
        0x0);

    if (!success) {
        printf("Failed to start file watcher\n");
    }
    assert(success);
}

MtFileWatcher *mt_file_watcher_create(
    MtAllocator *alloc, MtFileWatcherEventType types, const char *dir) {
    MtFileWatcher *w = mt_alloc(alloc, sizeof(MtFileWatcher));
    memset(w, 0, sizeof(*w));

    w->alloc = alloc;

    w->watch_dir     = mt_strdup(w->alloc, dir);
    w->watch_dir_len = strlen(w->watch_dir);

    w->directory = CreateFile(
        w->watch_dir,
        FILE_LIST_DIRECTORY,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        NULL,          // security descriptor
        OPEN_EXISTING, // how to create
        FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED, // file attributes
        NULL);

    watcher_begin_read(w);

    return w;
}

static char *build_full_path(MtFileWatcher *w, FILE_NOTIFY_INFORMATION *ev) {
    size_t name_len = (size_t)(ev->FileNameLength / 2);

    size_t length = w->watch_dir_len + 1 + name_len;
    char *res     = mt_alloc(w->alloc, length + 1);
    strcpy(res, w->watch_dir);
    res[w->watch_dir_len] = '/';

    // HACKY! convert to utf8 or keep as WCHAR depending on compile-define.
    for (size_t i = 0; i < name_len; ++i) {
        assert(ev->FileName[i] <= 255);
        res[w->watch_dir_len + 1 + i] =
            ev->FileName[i] > 255 ? '_' : (char)ev->FileName[i];
        if (res[w->watch_dir_len + 1 + i] == '\\') {
            res[w->watch_dir_len + 1 + i] = '/'; // switch slashes
        }
    }
    res[length] = '\0';
    return res;
}

void mt_file_watcher_poll(
    MtFileWatcher *w, MtFileWatcherHandler handler, void *user_data) {
    DWORD bytes;
    BOOL res = GetOverlappedResult(w->directory, &w->overlapped, &bytes, FALSE);

    if (res != TRUE) return;

    char *move_src = NULL;

    FILE_NOTIFY_INFORMATION *ev = (FILE_NOTIFY_INFORMATION *)w->read_buffer;
    while (1) {
        switch (ev->Action) {
        case FILE_ACTION_ADDED: {
            char *src            = build_full_path(w, ev);
            MtFileWatcherEvent e = {MT_FILE_WATCHER_EVENT_CREATE, src, NULL};
            handler(&e, user_data);
            mt_free(w->alloc, src);
        } break;
        case FILE_ACTION_REMOVED: {
            char *src            = build_full_path(w, ev);
            MtFileWatcherEvent e = {MT_FILE_WATCHER_EVENT_REMOVE, src, NULL};
            handler(&e, user_data);
            mt_free(w->alloc, src);
        } break;
        case FILE_ACTION_MODIFIED: {
            char *src            = build_full_path(w, ev);
            MtFileWatcherEvent e = {MT_FILE_WATCHER_EVENT_MODIFY, src, NULL};
            handler(&e, user_data);
            mt_free(w->alloc, src);
        } break;
        case FILE_ACTION_RENAMED_OLD_NAME: {
            move_src = build_full_path(w, ev);
        } break;
        case FILE_ACTION_RENAMED_NEW_NAME: {
            char *dst = build_full_path(w, ev);

            MtFileWatcherEvent e = {MT_FILE_WATCHER_EVENT_MOVE, move_src, dst};
            handler(&e, user_data);
            mt_free(w->alloc, dst);
            mt_free(w->alloc, move_src);
            move_src = NULL;
        } break;
        default: break;
        }

        if (ev->NextEntryOffset == 0) break;

        ev = (FILE_NOTIFY_INFORMATION *)((char *)ev + ev->NextEntryOffset);
    }

    if (move_src != NULL) {
        mt_free(w->alloc, move_src);
    }

    watcher_begin_read(w);
}

void mt_file_watcher_destroy(MtFileWatcher *w) {
    CloseHandle(w->directory);
    mt_free(w->alloc, w);
}
#endif
