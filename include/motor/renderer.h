#ifndef MOTOR_RENDERER_H
#define MOTOR_RENDERER_H

#include <stddef.h>
#include <stdbool.h>

typedef enum MtQueueType {
  MT_QUEUE_GRAPHICS,
  MT_QUEUE_COMPUTE,
  MT_QUEUE_TRANSFER,
} MtQueueType;

typedef struct MtRenderPass MtRenderPass;
typedef struct MtPipeline MtPipeline;
typedef struct MtBuffer MtBuffer;

typedef struct MtCmdBuffer MtCmdBuffer;

typedef struct MtBufferCreateInfo MtBufferCreateInfo;
typedef struct MtGraphicsPipelineCreateInfo MtGraphicsPipelineCreateInfo;

typedef struct MtCmdBufferVT {
  void (*begin)(MtCmdBuffer *);
  void (*end)(MtCmdBuffer *);

  void (*begin_render_pass)(MtCmdBuffer *, MtRenderPass *);
  void (*end_render_pass)(MtCmdBuffer *);

  void (*set_viewport)(MtCmdBuffer *, float x, float y, float w, float h);
  void (*set_scissor)(
      MtCmdBuffer *, int32_t x, int32_t y, uint32_t w, uint32_t h);

  void (*bind_pipeline)(MtCmdBuffer *, MtPipeline *pipeline);
} MtCmdBufferVT;

typedef struct MtICmdBuffer {
  MtCmdBuffer *inst;
  MtCmdBufferVT *vt;
} MtICmdBuffer;

typedef struct MtDevice MtDevice;

typedef struct MtDeviceVT {
  void (*begin_frame)(MtDevice *);

  void (*allocate_cmd_buffers)(
      MtDevice *, MtQueueType, uint32_t, MtICmdBuffer *);
  void (*free_cmd_buffers)(MtDevice *, MtQueueType, uint32_t, MtICmdBuffer *);

  MtBuffer *(*create_buffer)(MtDevice *, MtBufferCreateInfo *);
  void (*destroy_buffer)(MtDevice *, MtBuffer *);

  void *(*map_buffer)(MtDevice *, MtBuffer *);
  void (*unmap_buffer)(MtDevice *, MtBuffer *);

  void (*submit)(MtDevice *, MtICmdBuffer *);

  MtPipeline *(*create_graphics_pipeline)(
      MtDevice *,
      uint8_t *vertex_code,
      size_t vertex_code_size,
      uint8_t *fragment_code,
      size_t fragment_code_size,
      MtGraphicsPipelineCreateInfo *);
  void (*destroy_pipeline)(MtDevice *, MtPipeline *);

  void (*destroy)(MtDevice *);
} MtDeviceVT;

typedef struct MtIDevice {
  MtDevice *inst;
  MtDeviceVT *vt;
} MtIDevice;

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

typedef struct MtVertexInfo {
  MtVertexAttribute *attributes;
  uint32_t attribute_count;

  uint32_t stride;
} MtVertexInfo;

typedef struct MtGraphicsPipelineCreateInfo {
  bool blending;
  bool depth_test;
  bool depth_write;
  bool depth_bias;
  MtCullMode cull_mode;
  MtFrontFace front_face;
  float line_width;
  MtVertexInfo vertex_info;
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

#endif
