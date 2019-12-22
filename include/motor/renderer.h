#ifndef MOTOR_RENDERER_H
#define MOTOR_RENDERER_H

#include <stdbool.h>
#include "arena.h"

typedef struct MtDevice MtDevice;

typedef struct MtDeviceVT {
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
