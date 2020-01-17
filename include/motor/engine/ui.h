#pragma once

#include <stdint.h>
#include <motor/base/math_types.h>

typedef struct MtAllocator MtAllocator;
typedef struct MtFontAsset MtFontAsset;
typedef struct MtCmdBuffer MtCmdBuffer;
typedef struct MtImage MtImage;
typedef struct MtViewport MtViewport;
typedef struct MtAssetManager MtAssetManager;

typedef struct MtUIRenderer MtUIRenderer;

typedef struct MtUIVertex
{
    Vec2 pos;
    Vec2 tex_coords;
    Vec3 color;
} MtUIVertex;

MtUIRenderer *mt_ui_create(MtAllocator *alloc, MtAssetManager *asset_manager);
void mt_ui_destroy(MtUIRenderer *ui);

// Settings
void mt_ui_set_font(MtUIRenderer *ui, MtFontAsset *font);
void mt_ui_set_font_size(MtUIRenderer *ui, uint32_t height);
void mt_ui_set_pos(MtUIRenderer *ui, Vec2 pos);
void mt_ui_set_color(MtUIRenderer *ui, Vec3 color);

// Widgets
void mt_ui_print(MtUIRenderer *ui, const char *text);

#ifdef __GNUC__
__attribute__((format(printf, 2, 3)))
#endif
void mt_ui_printf(MtUIRenderer *ui, const char *fmt, ...);

void mt_ui_image(MtUIRenderer *ui, MtCmdBuffer *cb, MtImage *image);

// Drawing
void mt_ui_begin(MtUIRenderer *ui, MtViewport *viewport);
void mt_ui_draw(MtUIRenderer *ui, MtCmdBuffer *cb);
