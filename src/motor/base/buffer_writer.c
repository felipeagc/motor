#include <motor/base/buffer_writer.h>

#include <string.h>
#include <assert.h>
#include <motor/base/allocator.h>

void mt_buffer_writer_init(MtBufferWriter *bw, MtAllocator *alloc)
{
    memset(bw, 0, sizeof(*bw));

    bw->capacity = 1 << 14;
    bw->alloc = alloc;
    bw->buf = mt_alloc(bw->alloc, bw->capacity);
}

void mt_buffer_writer_reset(MtBufferWriter *bw)
{
    bw->length = 0;
}

void mt_buffer_writer_append(MtBufferWriter *bw, const void *data, size_t size)
{
    if (bw->length + size < bw->capacity)
    {
        size_t increase = MT_MAX(bw->capacity, size);
        bw->capacity += increase;
        bw->buf = mt_realloc(bw->alloc, bw->buf, bw->capacity);
    }

    memcpy(bw->buf + bw->length, data, size);
    bw->length += size;
}

uint8_t *mt_buffer_writer_build(MtBufferWriter *bw, size_t *out_size)
{
    *out_size = bw->length;

    if (bw->length == 0)
    {
        return NULL;
    }

    uint8_t *result = mt_alloc(bw->alloc, bw->length);
    memcpy(result, bw->buf, bw->length);
    return result;
}

void mt_buffer_writer_destroy(MtBufferWriter *bw)
{
    mt_free(bw->alloc, bw->buf);
}
