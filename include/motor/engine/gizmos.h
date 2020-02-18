#pragma once

#include "api_types.h"
#include <motor/base/math_types.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct MtCmdBuffer MtCmdBuffer;

MT_ENGINE_API void mt_draw_translation_gizmo(MtCmdBuffer *cb, const Vec3 *pos);

#ifdef __cplusplus
}
#endif
