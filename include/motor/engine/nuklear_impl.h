#pragma once

#include "api_types.h"

typedef struct MtNuklearContext MtNuklearContext;
typedef struct MtEngine MtEngine;
typedef struct MtEvent MtEvent;
typedef struct MtCmdBuffer MtCmdBuffer;

MT_ENGINE_API MtNuklearContext *mt_nuklear_create(MtEngine *engine);

MT_ENGINE_API void mt_nuklear_handle_event(MtNuklearContext *ctx, MtEvent *event);

MT_ENGINE_API void mt_nuklear_render(MtNuklearContext *ctx, MtCmdBuffer *cb);

MT_ENGINE_API struct nk_context *mt_nuklear_get_context(MtNuklearContext *ctx);

MT_ENGINE_API void mt_nuklear_destroy(MtNuklearContext *ctx);
