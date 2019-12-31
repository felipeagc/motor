#pragma once

#include <stdint.h>

typedef struct MtAllocator MtAllocator;

typedef struct MtStringBuilder {
    MtAllocator *alloc;
    char *buf;
    uint64_t capacity;
    uint64_t length;
} MtStringBuilder;

void mt_str_builder_init(MtStringBuilder *sb, MtAllocator *alloc);

void mt_str_builder_reset(MtStringBuilder *sb);

void mt_str_builder_append_str(MtStringBuilder *sb, char *str);

void mt_str_builder_append_char(MtStringBuilder *sb, char c);

char *mt_str_builder_build(MtStringBuilder *sb, MtAllocator *allocator);

void mt_str_builder_destroy(MtStringBuilder *sb);
