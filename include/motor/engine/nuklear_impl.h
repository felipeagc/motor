#pragma once

typedef struct MtNuklearContext MtNuklearContext;
typedef struct MtEngine MtEngine;
typedef struct MtEvent MtEvent;
typedef struct MtCmdBuffer MtCmdBuffer;

MtNuklearContext *mt_nuklear_create(MtEngine *engine);

void mt_nuklear_handle_event(MtNuklearContext *ctx, MtEvent *event);

void mt_nuklear_render(MtNuklearContext *ctx, MtCmdBuffer *cb);

struct nk_context *mt_nuklear_get_context(MtNuklearContext *ctx);

void mt_nuklear_destroy(MtNuklearContext *ctx);
