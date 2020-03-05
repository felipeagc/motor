#pragma once

#include "api_types.h"
#include <motor/base/log.h>
#include <motor/base/buffer_writer.h>
#include <motor/base/buffer_reader.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
    MT_SERIALIZE_TYPE_INVALID = 0x00,
    MT_SERIALIZE_TYPE_UINT8 = 0x01,
    MT_SERIALIZE_TYPE_UINT16 = 0x02,
    MT_SERIALIZE_TYPE_UINT32 = 0x03,
    MT_SERIALIZE_TYPE_UINT64 = 0x04,

    MT_SERIALIZE_TYPE_INT8 = 0x05,
    MT_SERIALIZE_TYPE_INT16 = 0x06,
    MT_SERIALIZE_TYPE_INT32 = 0x07,
    MT_SERIALIZE_TYPE_INT64 = 0x08,

    MT_SERIALIZE_TYPE_FLOAT32 = 0x09,
    MT_SERIALIZE_TYPE_FLOAT64 = 0x0A,

    MT_SERIALIZE_TYPE_VEC3 = 0x20,
    MT_SERIALIZE_TYPE_VEC4 = 0x21,
    MT_SERIALIZE_TYPE_QUAT = 0x22,
    MT_SERIALIZE_TYPE_MAT4 = 0x23,

    MT_SERIALIZE_TYPE_BUFFER = 0x40,
    MT_SERIALIZE_TYPE_STRING = 0x41,

    MT_SERIALIZE_TYPE_MAP = 0x60,
    MT_SERIALIZE_TYPE_ARRAY = 0x61,
};

typedef struct MtSerializeValue
{
    uint32_t type;
    union
    {
        uint8_t uint8;
        uint16_t uint16;
        uint32_t uint32;
        uint64_t uint64;

        int8_t int8;
        int16_t int16;
        int32_t int32;
        int64_t int64;

        float f32;
        double f64;

        Vec3 vec3;
        Vec4 vec4;
        Quat quat;
        Mat4 mat4;

        struct
        {
            const char *buf;
            uint32_t length;
        } str;

        struct
        {
            const uint8_t *buf;
            uint64_t size;
        } buf;

        struct
        {
            uint32_t pair_count;
        } map;

        struct
        {
            uint32_t element_count;
        } array;
    };
} MtSerializeValue;

//
// Serialization functions
//

static inline void mt_serialize_uint32(MtBufferWriter *bw, uint32_t val)
{
    uint32_t type = MT_SERIALIZE_TYPE_UINT32;
    mt_buffer_writer_append(bw, &type, sizeof(uint32_t));
    mt_buffer_writer_append(bw, &val, sizeof(uint32_t));
}

static inline void mt_serialize_int32(MtBufferWriter *bw, int32_t val)
{
    uint32_t type = MT_SERIALIZE_TYPE_INT32;
    mt_buffer_writer_append(bw, &type, sizeof(uint32_t));
    mt_buffer_writer_append(bw, &val, sizeof(int32_t));
}

static inline void mt_serialize_float32(MtBufferWriter *bw, float val)
{
    uint32_t type = MT_SERIALIZE_TYPE_FLOAT32;
    mt_buffer_writer_append(bw, &type, sizeof(uint32_t));
    mt_buffer_writer_append(bw, &val, sizeof(float));
}

static inline void mt_serialize_float64(MtBufferWriter *bw, double val)
{
    uint32_t type = MT_SERIALIZE_TYPE_FLOAT64;
    mt_buffer_writer_append(bw, &type, sizeof(uint32_t));
    mt_buffer_writer_append(bw, &val, sizeof(double));
}

static inline void mt_serialize_vec3(MtBufferWriter *bw, const Vec3 *val)
{
    uint32_t type = MT_SERIALIZE_TYPE_VEC3;
    mt_buffer_writer_append(bw, &type, sizeof(uint32_t));
    mt_buffer_writer_append(bw, val, sizeof(Vec3));
}

static inline void mt_serialize_vec4(MtBufferWriter *bw, const Vec4 *val)
{
    uint32_t type = MT_SERIALIZE_TYPE_VEC4;
    mt_buffer_writer_append(bw, &type, sizeof(uint32_t));
    mt_buffer_writer_append(bw, val, sizeof(Vec4));
}

static inline void mt_serialize_quat(MtBufferWriter *bw, const Quat *val)
{
    uint32_t type = MT_SERIALIZE_TYPE_QUAT;
    mt_buffer_writer_append(bw, &type, sizeof(uint32_t));
    mt_buffer_writer_append(bw, val, sizeof(Quat));
}

static inline void mt_serialize_mat4(MtBufferWriter *bw, const Mat4 *val)
{
    uint32_t type = MT_SERIALIZE_TYPE_MAT4;
    mt_buffer_writer_append(bw, &type, sizeof(uint32_t));
    mt_buffer_writer_append(bw, val, sizeof(Mat4));
}

static inline void mt_serialize_string(MtBufferWriter *bw, const char *val)
{
    uint32_t type = MT_SERIALIZE_TYPE_STRING;
    mt_buffer_writer_append(bw, &type, sizeof(uint32_t));

    uint32_t size = strlen(val) + 1;
    mt_buffer_writer_append(bw, &size, sizeof(uint32_t));
    mt_buffer_writer_append(bw, val, size - 1);
    mt_buffer_writer_append(bw, &(char){'\0'}, 1);
}

static inline void mt_serialize_buffer(MtBufferWriter *bw, const void *buf, uint64_t size)
{
    uint32_t type = MT_SERIALIZE_TYPE_BUFFER;
    mt_buffer_writer_append(bw, &type, sizeof(uint32_t));

    mt_buffer_writer_append(bw, &size, sizeof(uint64_t));
    mt_buffer_writer_append(bw, buf, size);
}

static inline void mt_serialize_map(MtBufferWriter *bw, uint32_t pair_count)
{
    uint32_t type = MT_SERIALIZE_TYPE_MAP;
    mt_buffer_writer_append(bw, &type, sizeof(uint32_t));
    mt_buffer_writer_append(bw, &pair_count, sizeof(uint32_t));
}

static inline void mt_serialize_array(MtBufferWriter *bw, uint32_t element_count)
{
    uint32_t type = MT_SERIALIZE_TYPE_ARRAY;
    mt_buffer_writer_append(bw, &type, sizeof(uint32_t));
    mt_buffer_writer_append(bw, &element_count, sizeof(uint32_t));
}

//
// Deserialization functions
//

static inline bool mt_deserialize_value(MtBufferReader *br, uint32_t type, MtSerializeValue *value)
{
    mt_buffer_reader_read(br, &value->type, sizeof(uint32_t));
    bool result = value->type == type;
    if (!result)
    {
        mt_log_warn("Serialization: wanted type: %u, got type: %u", type, value->type);
    }

    switch (value->type)
    {
        case MT_SERIALIZE_TYPE_UINT8:
            mt_buffer_reader_read(br, &value->uint8, sizeof(uint8_t));
            break;
        case MT_SERIALIZE_TYPE_UINT16:
            mt_buffer_reader_read(br, &value->uint16, sizeof(uint16_t));
            break;
        case MT_SERIALIZE_TYPE_UINT32:
            mt_buffer_reader_read(br, &value->uint32, sizeof(uint32_t));
            break;
        case MT_SERIALIZE_TYPE_UINT64:
            mt_buffer_reader_read(br, &value->uint64, sizeof(uint64_t));
            break;

        case MT_SERIALIZE_TYPE_INT8: mt_buffer_reader_read(br, &value->int8, sizeof(int8_t)); break;
        case MT_SERIALIZE_TYPE_INT16:
            mt_buffer_reader_read(br, &value->int16, sizeof(int16_t));
            break;
        case MT_SERIALIZE_TYPE_INT32:
            mt_buffer_reader_read(br, &value->int32, sizeof(int32_t));
            break;
        case MT_SERIALIZE_TYPE_INT64:
            mt_buffer_reader_read(br, &value->int64, sizeof(int64_t));
            break;

        case MT_SERIALIZE_TYPE_FLOAT32:
            mt_buffer_reader_read(br, &value->f32, sizeof(float));
            break;
        case MT_SERIALIZE_TYPE_FLOAT64:
            mt_buffer_reader_read(br, &value->f64, sizeof(double));
            break;

        case MT_SERIALIZE_TYPE_VEC3: mt_buffer_reader_read(br, &value->vec3, sizeof(Vec3)); break;
        case MT_SERIALIZE_TYPE_VEC4: mt_buffer_reader_read(br, &value->vec4, sizeof(Vec4)); break;
        case MT_SERIALIZE_TYPE_QUAT: mt_buffer_reader_read(br, &value->quat, sizeof(Quat)); break;
        case MT_SERIALIZE_TYPE_MAT4: mt_buffer_reader_read(br, &value->mat4, sizeof(Mat4)); break;

        case MT_SERIALIZE_TYPE_BUFFER:
            mt_buffer_reader_read(br, &value->buf.size, sizeof(uint64_t));
            value->buf.buf = mt_buffer_reader_at(br);
            mt_buffer_reader_read(br, NULL, value->buf.size);
            break;
        case MT_SERIALIZE_TYPE_STRING:
            mt_buffer_reader_read(br, &value->str.length, sizeof(uint32_t));
            value->str.buf = (const char *)mt_buffer_reader_at(br);
            mt_buffer_reader_read(br, NULL, value->str.length);
            break;

        case MT_SERIALIZE_TYPE_MAP:
            mt_buffer_reader_read(br, &value->map.pair_count, sizeof(uint32_t));
            break;
        case MT_SERIALIZE_TYPE_ARRAY:
            mt_buffer_reader_read(br, &value->array.element_count, sizeof(uint32_t));
            break;
    }

    return result;
}

#ifdef __cplusplus
}
#endif
