#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct MtDevice MtDevice;
typedef struct MtRenderPass MtRenderPass;
typedef struct MtPipeline MtPipeline;
typedef struct MtBuffer MtBuffer;
typedef struct MtCmdBuffer MtCmdBuffer;

typedef enum MtQueueType {
    MT_QUEUE_GRAPHICS,
    MT_QUEUE_COMPUTE,
    MT_QUEUE_TRANSFER,
} MtQueueType;

typedef enum MtFormat {
    MT_FORMAT_UNDEFINED,

    MT_FORMAT_R8_UINT,
    MT_FORMAT_R32_UINT,

    MT_FORMAT_RGB8_UNORM,
    MT_FORMAT_RGBA8_UNORM,

    MT_FORMAT_BGRA8_UNORM,

    MT_FORMAT_R32_SFLOAT,
    MT_FORMAT_RG32_SFLOAT,
    MT_FORMAT_RGB32_SFLOAT,
    MT_FORMAT_RGBA32_SFLOAT,

    MT_FORMAT_RGBA16_SFLOAT,

    MT_FORMAT_D16_UNORM,
    MT_FORMAT_D16_UNORM_S8_UINT,
    MT_FORMAT_D24_UNORM_S8_UINT,
    MT_FORMAT_D32_SFLOAT,
    MT_FORMAT_D32_SFLOAT_S8_UINT,
} MtFormat;

typedef enum MtIndexType {
    MT_INDEX_TYPE_UINT32,
    MT_INDEX_TYPE_UINT16,
} MtIndexType;

typedef enum MtCullMode {
    MT_CULL_MODE_NONE,
    MT_CULL_MODE_BACK,
    MT_CULL_MODE_FRONT,
    MT_CULL_MODE_FRONT_AND_BACK,
} MtCullMode;

typedef enum MtFrontFace {
    MT_FRONT_FACE_CLOCKWISE,
    MT_FRONT_FACE_COUNTER_CLOCKWISE,
} MtFrontFace;

typedef struct MtVertexAttribute {
    MtFormat format;
    uint32_t offset;
} MtVertexAttribute;

typedef struct MtGraphicsPipelineCreateInfo {
    bool blending;
    bool depth_test;
    bool depth_write;
    bool depth_bias;
    MtCullMode cull_mode;
    MtFrontFace front_face;
    float line_width;

    MtVertexAttribute *vertex_attributes;
    uint32_t vertex_attribute_count;
    uint32_t vertex_stride;
} MtGraphicsPipelineCreateInfo;

typedef union MtColor {
    struct {
        float r, g, b, a;
    };
    float values[4];
} MtColor;

typedef enum MtBufferUsage {
    MT_BUFFER_USAGE_VERTEX,
    MT_BUFFER_USAGE_INDEX,
    MT_BUFFER_USAGE_UNIFORM,
    MT_BUFFER_USAGE_STORAGE,
    MT_BUFFER_USAGE_TRANSFER,
} MtBufferUsage;

typedef enum MtBufferMemory {
    MT_BUFFER_MEMORY_HOST,
    MT_BUFFER_MEMORY_DEVICE,
} MtBufferMemory;

typedef struct MtBufferCreateInfo {
    MtBufferUsage usage;
    MtBufferMemory memory;
    size_t size;
} MtBufferCreateInfo;

typedef struct MtRenderer {
    void (*device_begin_frame)(MtDevice *);
    void (*destroy_device)(MtDevice *);

    void (*allocate_cmd_buffers)(
        MtDevice *, MtQueueType, uint32_t, MtCmdBuffer **);
    void (*free_cmd_buffers)(MtDevice *, MtQueueType, uint32_t, MtCmdBuffer **);

    MtBuffer *(*create_buffer)(MtDevice *, MtBufferCreateInfo *);
    void (*destroy_buffer)(MtDevice *, MtBuffer *);

    void *(*map_buffer)(MtDevice *, MtBuffer *);
    void (*unmap_buffer)(MtDevice *, MtBuffer *);

    void (*submit)(MtDevice *, MtCmdBuffer *);

    MtPipeline *(*create_graphics_pipeline)(
        MtDevice *,
        uint8_t *vertex_code,
        size_t vertex_code_size,
        uint8_t *fragment_code,
        size_t fragment_code_size,
        MtGraphicsPipelineCreateInfo *);
    MtPipeline *(*create_compute_pipeline)(
        MtDevice *, uint8_t *code, size_t code_size);
    void (*destroy_pipeline)(MtDevice *, MtPipeline *);

    void (*begin_cmd_buffer)(MtCmdBuffer *);
    void (*end_cmd_buffer)(MtCmdBuffer *);

    void (*cmd_begin_render_pass)(MtCmdBuffer *, MtRenderPass *);
    void (*cmd_end_render_pass)(MtCmdBuffer *);

    void (*cmd_set_viewport)(MtCmdBuffer *, float x, float y, float w, float h);
    void (*cmd_set_scissor)(
        MtCmdBuffer *, int32_t x, int32_t y, uint32_t w, uint32_t h);

    void (*cmd_set_uniform)(
        MtCmdBuffer *, uint32_t set, uint32_t binding, void *data, size_t size);

    void (*cmd_bind_pipeline)(MtCmdBuffer *, MtPipeline *pipeline);
    void (*cmd_bind_descriptor_set)(MtCmdBuffer *, uint32_t set);

    void (*cmd_bind_vertex_buffer)(MtCmdBuffer *, MtBuffer *, size_t offset);
    void (*cmd_bind_index_buffer)(
        MtCmdBuffer *, MtBuffer *, MtIndexType index_type, size_t offset);

    void (*cmd_bind_vertex_data)(MtCmdBuffer *, void *data, size_t size);
    void (*cmd_bind_index_data)(
        MtCmdBuffer *, void *data, size_t size, MtIndexType index_type);

    void (*cmd_draw)(
        MtCmdBuffer *, uint32_t vertex_count, uint32_t instance_count);
    void (*cmd_draw_indexed)(
        MtCmdBuffer *, uint32_t index_count, uint32_t instance_count);
} MtRenderer;

extern MtRenderer mt_render;
