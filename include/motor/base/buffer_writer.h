#pragma once

#include "api_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct MtAllocator MtAllocator;

typedef struct MtBufferWriter
{
    MtAllocator *alloc;
    uint8_t *buf;
    size_t capacity;
    size_t length;
} MtBufferWriter;

MT_BASE_API void mt_buffer_writer_init(MtBufferWriter *bw, MtAllocator *alloc);

MT_BASE_API void mt_buffer_writer_reset(MtBufferWriter *bw);

MT_BASE_API void mt_buffer_writer_append(MtBufferWriter *bw, const void *data, size_t size);

MT_BASE_API uint8_t *mt_buffer_writer_build(MtBufferWriter *bw, size_t *out_size);

MT_BASE_API void mt_buffer_writer_destroy(MtBufferWriter *bw);

#ifdef __cplusplus
}
#endif
