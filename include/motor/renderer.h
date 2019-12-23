#ifndef MOTOR_RENDERER_H
#define MOTOR_RENDERER_H

#include <stdbool.h>
#include "arena.h"

typedef enum MtQueueType {
  MT_QUEUE_GRAPHICS,
  MT_QUEUE_COMPUTE,
  MT_QUEUE_TRANSFER,
} MtQueueType;

typedef struct MtRenderPass MtRenderPass;

typedef struct MtCmdBuffer MtCmdBuffer;

typedef struct MtCmdBufferVT {
  void (*begin)(MtCmdBuffer *);
  void (*end)(MtCmdBuffer *);

  void (*begin_render_pass)(MtCmdBuffer *, MtRenderPass *);
  void (*end_render_pass)(MtCmdBuffer *);

  void (*set_viewport)(MtCmdBuffer *, float x, float y, float w, float h);
  void (*set_scissor)(
      MtCmdBuffer *, int32_t x, int32_t y, uint32_t w, uint32_t h);
} MtCmdBufferVT;

typedef struct MtICmdBuffer {
  MtCmdBuffer *inst;
  MtCmdBufferVT *vt;
} MtICmdBuffer;

typedef struct MtDevice MtDevice;

typedef struct MtDeviceVT {
  void (*allocate_cmd_buffers)(
      MtDevice *, MtQueueType, uint32_t, MtICmdBuffer *);
  void (*free_cmd_buffers)(MtDevice *, MtQueueType, uint32_t, MtICmdBuffer *);
  void (*submit)(MtDevice *, MtICmdBuffer *);
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

typedef union MtColor {
  struct {
    float r, g, b, a;
  };
  float values[4];
} MtColor;

#endif
