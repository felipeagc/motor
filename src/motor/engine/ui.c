#include <motor/engine/ui.h>

#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <motor/base/math.h>
#include <motor/base/array.h>
#include <motor/base/allocator.h>
#include <motor/graphics/renderer.h>
#include <motor/engine/asset_manager.h>
#include <motor/engine/assets/pipeline_asset.h>
#include <motor/engine/assets/font_asset.h>
#include "assets/font_asset.inl"

struct MtUIRenderer
{
    MtAllocator *alloc;

    MtPipelineAsset *pipeline;

    MtFontAsset *font;
    uint32_t font_height;

    Vec2 pos;
    Vec3 color;

    MtViewport viewport;

    MtUIVertex *vertices;
    uint16_t *indices;
};

MtUIRenderer *mt_ui_create(MtAllocator *alloc, MtAssetManager *asset_manager)
{
    MtUIRenderer *ui = mt_alloc(alloc, sizeof(MtUIRenderer));
    memset(ui, 0, sizeof(*ui));

    ui->alloc = alloc;

    ui->pipeline =
        (MtPipelineAsset *)mt_asset_manager_load(asset_manager, "../assets/shaders/ui.glsl");
    assert(ui->pipeline);

    ui->font_height = 20;
    ui->pos         = V2(0.0f, 0.0f);
    ui->color       = V3(1.0f, 1.0f, 1.0f);

    return ui;
}

void mt_ui_destroy(MtUIRenderer *ui)
{
    mt_free(ui->alloc, ui);
}

void mt_ui_set_font(MtUIRenderer *ui, MtFontAsset *font)
{
    ui->font = font;
}

void mt_ui_set_font_size(MtUIRenderer *ui, uint32_t height)
{
    ui->font_height = height;
}

void mt_ui_set_pos(MtUIRenderer *ui, Vec2 pos)
{
    ui->pos = pos;
}

void mt_ui_set_color(MtUIRenderer *ui, Vec3 color)
{
    ui->color = color;
}

static void draw_text(MtUIRenderer *ui, const char *text)
{
    FontAtlas *atlas = get_atlas(ui->font, ui->font_height);

    ui->pos.y += (float)ui->font_height;

    float last_x = 0.0f;

    uint32_t ci = 0;
    while (text[ci])
    {
        stbtt_packedchar *cd = &atlas->chardata[text[ci]];

        uint16_t first_vertex = mt_array_size(ui->vertices);

        float x0 = (float)cd->x0 / (float)atlas->dim;
        float x1 = (float)cd->x1 / (float)atlas->dim;
        float y0 = (float)cd->y0 / (float)atlas->dim;
        float y1 = (float)cd->y1 / (float)atlas->dim;

        MtUIVertex vertices[4] = {
            {
                {last_x + ui->pos.x, ui->pos.y},
                {x0, y1},
                ui->color,
            }, // Bottom Left
            {
                {last_x + ui->pos.x, cd->yoff + ui->pos.y},
                {x0, y0},
                ui->color,
            }, // Top Left
            {
                {last_x + cd->xoff2 + ui->pos.x, cd->yoff + ui->pos.y},
                {x1, y0},
                ui->color,
            }, // Top Right
            {
                {last_x + cd->xoff2 + ui->pos.x, ui->pos.y},
                {x1, y1},
                ui->color,
            }, // Bottom Right
        };

        mt_array_push(ui->alloc, ui->vertices, vertices[0]);
        mt_array_push(ui->alloc, ui->vertices, vertices[1]);
        mt_array_push(ui->alloc, ui->vertices, vertices[2]);
        mt_array_push(ui->alloc, ui->vertices, vertices[3]);

        mt_array_push(ui->alloc, ui->indices, first_vertex);
        mt_array_push(ui->alloc, ui->indices, first_vertex + 1);
        mt_array_push(ui->alloc, ui->indices, first_vertex + 2);
        mt_array_push(ui->alloc, ui->indices, first_vertex + 2);
        mt_array_push(ui->alloc, ui->indices, first_vertex + 3);
        mt_array_push(ui->alloc, ui->indices, first_vertex);

        last_x += cd->xadvance;

        ci++;
    }
}

void mt_ui_printf(MtUIRenderer *ui, const char *fmt, ...)
{
    char buf[512];
    va_list vl;
    va_start(vl, fmt);
    vsnprintf(buf, sizeof(buf) - 1, fmt, vl);
    va_end(vl);

    buf[sizeof(buf) - 1] = '\0';

    draw_text(ui, buf);
}

void mt_ui_begin(MtUIRenderer *ui, MtViewport *viewport)
{
    ui->viewport = *viewport;
    mt_array_set_size(ui->vertices, 0);
    mt_array_set_size(ui->indices, 0);
    ui->pos   = V2(0.0f, 0.0f);
    ui->color = V3(1.0f, 1.0f, 1.0f);
}

void mt_ui_draw(MtUIRenderer *ui, MtCmdBuffer *cb)
{
    Mat4 transform = mat4_orthographic(
        0.0f, (float)ui->viewport.width, 0.0f, (float)ui->viewport.height, 0.0f, 1.0f);

    mt_render.cmd_bind_pipeline(cb, ui->pipeline->pipeline);

    mt_render.cmd_bind_uniform(cb, &transform, sizeof(transform), 0, 0);

    FontAtlas *atlas = get_atlas(ui->font, ui->font_height);

    mt_render.cmd_bind_image(cb, atlas->image, ui->font->sampler, 0, 1);

    mt_render.cmd_bind_vertex_data(
        cb, ui->vertices, mt_array_size(ui->vertices) * sizeof(MtUIVertex));
    mt_render.cmd_bind_index_data(
        cb, ui->indices, mt_array_size(ui->indices) * sizeof(uint16_t), MT_INDEX_TYPE_UINT16);

    mt_render.cmd_draw_indexed(cb, mt_array_size(ui->indices), 1, 0, 0, 0);
}
