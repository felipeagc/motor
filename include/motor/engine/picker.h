#pragma once

#include "api_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct MtEngine MtEngine;
typedef struct MtCmdBuffer MtCmdBuffer;
typedef struct MtCameraUniform MtCameraUniform;

typedef struct MtPicker MtPicker;

typedef void (*MtPickerDrawCallback)(MtCmdBuffer *, void *user_data);

MT_ENGINE_API
MtPicker *mt_picker_create(MtEngine *engine);

MT_ENGINE_API void mt_picker_destroy(MtPicker *picker);

MT_ENGINE_API void mt_picker_resize(MtPicker *picker);

MT_ENGINE_API uint32_t mt_picker_pick(
    MtPicker *picker,
    const MtCameraUniform *camera_uniform,
    int32_t x,
    int32_t y,
    MtPickerDrawCallback draw,
    void *user_data);

#ifdef __cplusplus
}
#endif
