#include <motor/base/log.h>

#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <motor/base/threads.h>

static const char *level_names[]  = {"INFO", "DEBUG", "WARN", "ERROR", "FATAL"};
static const char *level_colors[] = {"\x1b[32m", "\x1b[36m", "\x1b[33m", "\x1b[31m", "\x1b[35m"};

static MtMutex global_mutex;

void mt_log_init()
{
    mt_mutex_init(&global_mutex);
}

void mt_log_log(MtLogLevel level, const char *filename, uint32_t line, const char *fmt, ...)
{
    mt_mutex_lock(&global_mutex);

    time_t t      = time(NULL);
    struct tm *lt = localtime(&t);

    char buf[16];
    buf[strftime(buf, sizeof(buf), "%H:%M:%S", lt)] = '\0';

#if defined(NDBEUG)
    fprintf(stderr, "%s %s%-5s\x1b[0m \x1b[0m ", buf, level_colors[level], level_names[level]);
#else
    fprintf(
        stderr,
        "%s %s%-5s\x1b[0m \x1b[90m%s:%d:\x1b[0m ",
        buf,
        level_colors[level],
        level_names[level],
        filename,
        line);
#endif

    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fprintf(stderr, "\n");
    fflush(stderr);

    mt_mutex_unlock(&global_mutex);
}
