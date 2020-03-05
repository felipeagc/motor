#pragma once

#include "api_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct MtAllocator MtAllocator;

typedef struct MtBufferReader
{
    const uint8_t *buf;
    size_t size;
    size_t pos;
} MtBufferReader;

static inline void mt_buffer_reader_init(MtBufferReader *br, const uint8_t *buf, size_t size)
{
    br->buf = buf;
    br->size = size;
    br->pos = 0;
}

static inline const uint8_t *mt_buffer_reader_at(MtBufferReader *br)
{
    return br->buf + br->pos;
}

static inline bool mt_buffer_reader_read(MtBufferReader *br, void *read_to, size_t size)
{
    if (br->pos + size >= br->size)
    {
        return false;
    }

    if (read_to)
    {
        memcpy(read_to, br->buf + br->pos, size);
    }
    br->pos += size;

    return true;
}

#ifdef __cplusplus
}
#endif
