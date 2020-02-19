#pragma once

#include "api_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct MtEngine MtEngine;
typedef struct MtCmdBuffer MtCmdBuffer;
typedef struct MtCameraUniform MtCameraUniform;

typedef enum MtPickerSelectionType {
    MT_PICKER_SELECTION_LAST = UINT32_MAX - 5,
    MT_PICKER_SELECTION_TRANSLATE_X = UINT32_MAX - 4,
    MT_PICKER_SELECTION_TRANSLATE_Y = UINT32_MAX - 3,
    MT_PICKER_SELECTION_TRANSLATE_Z = UINT32_MAX - 2,
    MT_PICKER_SELECTION_INALID = UINT32_MAX - 1,
    MT_PICKER_SELECTION_DESELECT = UINT32_MAX,
} MtPickerSelectionType;

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
