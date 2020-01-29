#pragma once

#include <stdint.h>
#include <motor/base/util.h>

typedef enum MtLogLevel
{
    MT_LOG_INFO,
    MT_LOG_DEBUG,
    MT_LOG_WARN,
    MT_LOG_ERROR,
    MT_LOG_FATAL,
} MtLogLevel;

#define mt_log_info(...) mt_log_log(MT_LOG_INFO, __FILE__, __LINE__, __VA_ARGS__)
#define mt_log_debug(...) mt_log_log(MT_LOG_DEBUG, __FILE__, __LINE__, __VA_ARGS__)
#define mt_log_warn(...) mt_log_log(MT_LOG_WARN, __FILE__, __LINE__, __VA_ARGS__)
#define mt_log_error(...) mt_log_log(MT_LOG_ERROR, __FILE__, __LINE__, __VA_ARGS__)
#define mt_log_fatal(...) mt_log_log(MT_LOG_FATAL, __FILE__, __LINE__, __VA_ARGS__)

#if defined(NDEBUG)
#undef mt_log_debug
#define mt_log_debug(...)
#endif

#define mt_log mt_log_info

void mt_log_init();

MT_PRINTF_FORMATTING(4, 5)
void mt_log_log(MtLogLevel level, const char *filename, uint32_t line, const char *fmt, ...);
