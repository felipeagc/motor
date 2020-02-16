#pragma once

#include "api_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct MtAllocator MtAllocator;

typedef struct MtStringBuilder
{
    MtAllocator *alloc;
    char *buf;
    uint64_t capacity;
    uint64_t length;
} MtStringBuilder;

MT_BASE_API void mt_str_builder_init(MtStringBuilder *sb, MtAllocator *alloc);

MT_BASE_API void mt_str_builder_reset(MtStringBuilder *sb);

MT_BASE_API void mt_str_builder_append_str(MtStringBuilder *sb, const char *str);

MT_BASE_API void mt_str_builder_append_strn(MtStringBuilder *sb, const char *str, size_t length);

MT_BASE_API void mt_str_builder_append_char(MtStringBuilder *sb, char c);

MT_BASE_API char *mt_str_builder_build(MtStringBuilder *sb, MtAllocator *allocator);

MT_BASE_API void mt_str_builder_destroy(MtStringBuilder *sb);

#ifdef __cplusplus
}
#endif
