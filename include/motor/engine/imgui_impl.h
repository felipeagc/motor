#pragma once

#include "api_types.h"
#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include <motor/engine/cimgui.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct MtEngine MtEngine;
typedef struct MtImguiContext MtImguiContext;
typedef struct MtEvent MtEvent;
typedef struct MtCmdBuffer MtCmdBuffer;

MT_ENGINE_API MtImguiContext *mt_imgui_create(MtEngine *engine);

MT_ENGINE_API void mt_imgui_handle_event(MtImguiContext *ctx, MtEvent *event);

MT_ENGINE_API void mt_imgui_begin(MtImguiContext *ctx);

MT_ENGINE_API void mt_imgui_render(MtImguiContext *ctx, MtCmdBuffer *cb);

MT_ENGINE_API void mt_imgui_destroy(MtImguiContext *ctx);

#ifdef __cplusplus
}
#endif
