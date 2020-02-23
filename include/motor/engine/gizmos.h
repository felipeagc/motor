#pragma once

#include "api_types.h"
#include <motor/base/math_types.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct MtCmdBuffer MtCmdBuffer;
typedef struct MtCameraUniform MtCameraUniform;
typedef struct MtWindow MtWindow;
typedef struct MtEvent MtEvent;

typedef enum MtGizmoState {
    MT_GIZMO_STATE_NONE = 0,
    MT_GIZMO_STATE_TRANSLATE,
} MtGizmoState;

typedef struct MtGizmo
{
    bool hovered;
    uint32_t axis;
    MtGizmoState state;
    Vec3 previous_intersect;
    Vec3 current_intersect;
} MtGizmo;

MT_ENGINE_API void mt_translation_gizmo_draw(
    MtGizmo *gizmo, MtCmdBuffer *cb, MtWindow *window, const MtCameraUniform *camera, Vec3 *pos);

#ifdef __cplusplus
}
#endif
