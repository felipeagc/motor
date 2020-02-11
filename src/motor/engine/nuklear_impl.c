#include <motor/engine/nuklear_impl.h>

#include <assert.h>
#include <string.h>
#include <stdbool.h>
#include <motor/base/log.h>
#include <motor/base/math.h>
#include <motor/base/allocator.h>
#include <motor/graphics/renderer.h>
#include <motor/graphics/window.h>
#include <motor/engine/engine.h>
#include <motor/engine/asset_manager.h>
#include <motor/engine/assets/pipeline_asset.h>
#include <motor/engine/nuklear.h>

#define MAX_VERTEX_MEMORY 512 * 1024
#define MAX_INDEX_MEMORY 128 * 1024

struct MtNuklearContext
{
    struct nk_context ctx;
    struct nk_buffer cmds;
    struct nk_font_atlas atlas;
    struct nk_draw_null_texture null;

    MtEngine *engine;
    MtPipelineAsset *pipeline;
    MtImage *font_image;
    MtSampler *sampler;
};

typedef struct NkVertex
{
    float position[2];
    float uv[2];
    float col[4];
} NkVertex;

static void font_stash_begin(MtNuklearContext *ctx)
{
    nk_font_atlas_init_default(&ctx->atlas);
    nk_font_atlas_begin(&ctx->atlas);
}

static void font_stash_end(MtNuklearContext *ctx)
{
    const void *image;
    int w, h;
    image = nk_font_atlas_bake(&ctx->atlas, &w, &h, NK_FONT_ATLAS_RGBA32);

    ctx->font_image = mt_render.create_image(
        ctx->engine->device,
        &(MtImageCreateInfo){
            .format = MT_FORMAT_RGBA8_UNORM,
            .width = (uint32_t)w,
            .height = (uint32_t)h,
            .usage = MT_IMAGE_USAGE_SAMPLED_BIT | MT_IMAGE_USAGE_TRANSFER_DST_BIT,
        });

    mt_render.transfer_to_image(
        ctx->engine->device,
        &(MtImageCopyView){.image = ctx->font_image},
        (size_t)(w * h * 4),
        image);

    nk_font_atlas_end(&ctx->atlas, nk_handle_ptr(ctx->font_image), &ctx->null);
    if (ctx->atlas.default_font)
    {
        nk_style_set_font(&ctx->ctx, &ctx->atlas.default_font->handle);
    }
}

MtNuklearContext *mt_nuklear_create(MtEngine *engine)
{
    MtNuklearContext *ctx = mt_alloc(engine->alloc, sizeof(*ctx));
    memset(ctx, 0, sizeof(*ctx));
    ctx->engine = engine;

    nk_init_default(&ctx->ctx, 0);

    ctx->pipeline = (MtPipelineAsset *)mt_asset_manager_load(
        &engine->asset_manager, "../assets/shaders/ui.hlsl");
    assert(ctx->pipeline);

    ctx->sampler = mt_render.create_sampler(
        ctx->engine->device,
        &(MtSamplerCreateInfo){.mag_filter = MT_FILTER_NEAREST, .min_filter = MT_FILTER_NEAREST});

    font_stash_begin(ctx);
    struct nk_font *font =
        nk_font_atlas_add_from_file(&ctx->atlas, "../assets/fonts/SourceSansPro-Regular.ttf", 18, 0);
    font_stash_end(ctx);

    nk_style_set_font(&ctx->ctx, &font->handle);

    nk_buffer_init_default(&ctx->cmds);

    struct nk_color table[NK_COLOR_COUNT];
    table[NK_COLOR_TEXT] = nk_rgba(190, 190, 190, 255);
    table[NK_COLOR_WINDOW] = nk_rgba(30, 33, 40, 245);
    table[NK_COLOR_HEADER] = nk_rgba(181, 45, 69, 220);
    table[NK_COLOR_BORDER] = nk_rgba(51, 55, 67, 255);
    table[NK_COLOR_BUTTON] = nk_rgba(181, 45, 69, 255);
    table[NK_COLOR_BUTTON_HOVER] = nk_rgba(190, 50, 70, 255);
    table[NK_COLOR_BUTTON_ACTIVE] = nk_rgba(195, 55, 75, 255);
    table[NK_COLOR_TOGGLE] = nk_rgba(51, 55, 67, 255);
    table[NK_COLOR_TOGGLE_HOVER] = nk_rgba(45, 60, 60, 255);
    table[NK_COLOR_TOGGLE_CURSOR] = nk_rgba(181, 45, 69, 255);
    table[NK_COLOR_SELECT] = nk_rgba(51, 55, 67, 255);
    table[NK_COLOR_SELECT_ACTIVE] = nk_rgba(181, 45, 69, 255);
    table[NK_COLOR_SLIDER] = nk_rgba(51, 55, 67, 255);
    table[NK_COLOR_SLIDER_CURSOR] = nk_rgba(181, 45, 69, 255);
    table[NK_COLOR_SLIDER_CURSOR_HOVER] = nk_rgba(186, 50, 74, 255);
    table[NK_COLOR_SLIDER_CURSOR_ACTIVE] = nk_rgba(191, 55, 79, 255);
    table[NK_COLOR_PROPERTY] = nk_rgba(51, 55, 67, 255);
    table[NK_COLOR_EDIT] = nk_rgba(51, 55, 67, 225);
    table[NK_COLOR_EDIT_CURSOR] = nk_rgba(190, 190, 190, 255);
    table[NK_COLOR_COMBO] = nk_rgba(51, 55, 67, 255);
    table[NK_COLOR_CHART] = nk_rgba(51, 55, 67, 255);
    table[NK_COLOR_CHART_COLOR] = nk_rgba(170, 40, 60, 255);
    table[NK_COLOR_CHART_COLOR_HIGHLIGHT] = nk_rgba(255, 0, 0, 255);
    table[NK_COLOR_SCROLLBAR] = nk_rgba(30, 33, 40, 255);
    table[NK_COLOR_SCROLLBAR_CURSOR] = nk_rgba(64, 84, 95, 255);
    table[NK_COLOR_SCROLLBAR_CURSOR_HOVER] = nk_rgba(70, 90, 100, 255);
    table[NK_COLOR_SCROLLBAR_CURSOR_ACTIVE] = nk_rgba(75, 95, 105, 255);
    table[NK_COLOR_TAB_HEADER] = nk_rgba(181, 45, 69, 220);
    nk_style_from_table(&ctx->ctx, table);

    return ctx;
}

void mt_nuklear_handle_event(MtNuklearContext *nk_ctx, MtEvent *event)
{
    struct nk_context *ctx = &nk_ctx->ctx;
    if (event->type == MT_EVENT_KEY_PRESSED || event->type == MT_EVENT_KEY_RELEASED)
    {
        /* key events */
        bool down = event->type == MT_EVENT_KEY_PRESSED;
        bool ctrl = mt_window.get_key(event->window, MT_KEY_LEFT_CONTROL) == MT_INPUT_STATE_PRESS;
        MtKey sym = event->keyboard.key;
        if (sym == MT_KEY_RIGHT_SHIFT || sym == MT_KEY_LEFT_SHIFT)
            nk_input_key(ctx, NK_KEY_SHIFT, down);
        else if (sym == MT_KEY_DELETE)
            nk_input_key(ctx, NK_KEY_DEL, down);
        else if (sym == MT_KEY_ENTER)
            nk_input_key(ctx, NK_KEY_ENTER, down);
        else if (sym == MT_KEY_TAB)
            nk_input_key(ctx, NK_KEY_TAB, down);
        else if (sym == MT_KEY_BACKSPACE)
            nk_input_key(ctx, NK_KEY_BACKSPACE, down);
        else if (sym == MT_KEY_HOME)
        {
            nk_input_key(ctx, NK_KEY_TEXT_START, down);
            nk_input_key(ctx, NK_KEY_SCROLL_START, down);
        }
        else if (sym == MT_KEY_END)
        {
            nk_input_key(ctx, NK_KEY_TEXT_END, down);
            nk_input_key(ctx, NK_KEY_SCROLL_END, down);
        }
        else if (sym == MT_KEY_PAGE_DOWN)
        {
            nk_input_key(ctx, NK_KEY_SCROLL_DOWN, down);
        }
        else if (sym == MT_KEY_PAGE_UP)
        {
            nk_input_key(ctx, NK_KEY_SCROLL_UP, down);
        }
        else if (sym == MT_KEY_Z)
            nk_input_key(ctx, NK_KEY_TEXT_UNDO, down && ctrl);
        else if (sym == MT_KEY_R)
            nk_input_key(ctx, NK_KEY_TEXT_REDO, down && ctrl);
        else if (sym == MT_KEY_C)
            nk_input_key(ctx, NK_KEY_COPY, down && ctrl);
        else if (sym == MT_KEY_V)
            nk_input_key(ctx, NK_KEY_PASTE, down && ctrl);
        else if (sym == MT_KEY_X)
            nk_input_key(ctx, NK_KEY_CUT, down && ctrl);
        else if (sym == MT_KEY_B)
            nk_input_key(ctx, NK_KEY_TEXT_LINE_START, down && ctrl);
        else if (sym == MT_KEY_E)
            nk_input_key(ctx, NK_KEY_TEXT_LINE_END, down && ctrl);
        else if (sym == MT_KEY_UP)
            nk_input_key(ctx, NK_KEY_UP, down);
        else if (sym == MT_KEY_DOWN)
            nk_input_key(ctx, NK_KEY_DOWN, down);
        else if (sym == MT_KEY_LEFT)
        {
            if (ctrl)
                nk_input_key(ctx, NK_KEY_TEXT_WORD_LEFT, down);
            else
                nk_input_key(ctx, NK_KEY_LEFT, down);
        }
        else if (sym == MT_KEY_RIGHT)
        {
            if (ctrl)
                nk_input_key(ctx, NK_KEY_TEXT_WORD_RIGHT, down);
            else
                nk_input_key(ctx, NK_KEY_RIGHT, down);
        }
        else
            return;
        return;
    }
    else if (event->type == MT_EVENT_BUTTON_PRESSED || event->type == MT_EVENT_BUTTON_RELEASED)
    {
        /* mouse button */
        bool down = event->type == MT_EVENT_BUTTON_PRESSED;
        double fx, fy;
        mt_window.get_cursor_pos(event->window, &fx, &fy);
        int x = (int)fx;
        int y = (int)fy;
        if (event->mouse.button == MT_MOUSE_BUTTON_LEFT)
        {
            /* if (event->button.clicks > 1) */
            /*     nk_input_button(ctx, NK_BUTTON_DOUBLE, x, y, down); */
            nk_input_button(ctx, NK_BUTTON_LEFT, x, y, down);
        }
        else if (event->mouse.button == MT_MOUSE_BUTTON_MIDDLE)
            nk_input_button(ctx, NK_BUTTON_MIDDLE, x, y, down);
        else if (event->mouse.button == MT_MOUSE_BUTTON_RIGHT)
            nk_input_button(ctx, NK_BUTTON_RIGHT, x, y, down);
        return;
    }
    else if (event->type == MT_EVENT_CURSOR_MOVED)
    {
        /* mouse motion */
        if (ctx->input.mouse.grabbed)
        {
            // TODO
            /* int x = (int)ctx->input.mouse.prev.x, y = (int)ctx->input.mouse.prev.y; */
            /* nk_input_motion(ctx, x + event->motion.xrel, y + event->motion.yrel); */
        }
        else
            nk_input_motion(ctx, event->pos.x, event->pos.y);
        return;
    }
    else if (event->type == MT_EVENT_CODEPOINT_INPUT)
    {
        /* text input */
        nk_glyph glyph;
        memcpy(glyph, &event->codepoint, NK_UTF_SIZE);
        nk_input_glyph(ctx, glyph);
        return;
    }
    else if (event->type == MT_EVENT_SCROLLED)
    {
        /* mouse wheel */
        nk_input_scroll(ctx, nk_vec2((float)event->scroll.x, (float)event->scroll.y));
        return;
    }
}

void mt_nuklear_render(MtNuklearContext *ctx, MtCmdBuffer *cb)
{
    uint32_t width, height;
    mt_window.get_size(ctx->engine->window, &width, &height);

    Mat4 ortho = mat4_orthographic(0.0f, (float)width, 0.0f, (float)height, 0.0f, 1.0f);

    struct nk_vec2 scale;
    scale.x = 1.0f;
    scale.y = 1.0f;

    mt_render.cmd_bind_pipeline(cb, ctx->pipeline->pipeline);
    mt_render.cmd_set_viewport(cb, &(MtViewport){.width = (float)width, .height = (float)height});

    mt_render.cmd_bind_uniform(cb, &ortho, sizeof(ortho), 0, 0);
    mt_render.cmd_bind_sampler(cb, ctx->sampler, 0, 1);

    /* convert from command queue into draw list and draw to screen */
    const struct nk_draw_command *cmd;
    void *vertices, *indices;
    uint32_t offset = 0;
    struct nk_buffer vbuf, ibuf;

    vertices = mt_render.cmd_bind_vertex_data(cb, MAX_VERTEX_MEMORY);
    indices = mt_render.cmd_bind_index_data(cb, MAX_INDEX_MEMORY, MT_INDEX_TYPE_UINT16);
    {
        /* fill convert configuration */
        struct nk_convert_config config = {0};
        static const struct nk_draw_vertex_layout_element vertex_layout[] = {
            {NK_VERTEX_POSITION, NK_FORMAT_FLOAT, NK_OFFSETOF(NkVertex, position)},
            {NK_VERTEX_TEXCOORD, NK_FORMAT_FLOAT, NK_OFFSETOF(NkVertex, uv)},
            {NK_VERTEX_COLOR, NK_FORMAT_R32G32B32A32_FLOAT, NK_OFFSETOF(NkVertex, col)},
            {NK_VERTEX_LAYOUT_END}};
        memset(&config, 0, sizeof(config));
        config.vertex_layout = vertex_layout;
        config.vertex_size = sizeof(NkVertex);
        config.vertex_alignment = NK_ALIGNOF(NkVertex);
        config.null = ctx->null;
        config.circle_segment_count = 22;
        config.curve_segment_count = 22;
        config.arc_segment_count = 22;
        config.global_alpha = 1.0f;
        config.shape_AA = NK_ANTI_ALIASING_ON;
        config.line_AA = NK_ANTI_ALIASING_ON;

        /* setup buffers to load vertices and indices */
        nk_buffer_init_fixed(&vbuf, vertices, (nk_size)MAX_VERTEX_MEMORY);
        nk_buffer_init_fixed(&ibuf, indices, (nk_size)MAX_INDEX_MEMORY);
        nk_convert(&ctx->ctx, &ctx->cmds, &vbuf, &ibuf, &config);
    }

    /* iterate over and execute each draw command */
    nk_draw_foreach(cmd, &ctx->ctx, &ctx->cmds)
    {
        if (!cmd->elem_count)
            continue;
        mt_render.cmd_bind_image(cb, (MtImage *)cmd->texture.ptr, 0, 2);
        mt_render.cmd_set_scissor(
            cb,
            MT_MAX((int32_t)(cmd->clip_rect.x * scale.x), 0),
            MT_MAX((int32_t)(cmd->clip_rect.y * scale.y), 0),
            (uint32_t)(cmd->clip_rect.w * scale.x),
            (uint32_t)(cmd->clip_rect.h * scale.y));
        mt_render.cmd_draw_indexed(cb, (uint32_t)cmd->elem_count, 1, offset, 0, 0);
        offset += cmd->elem_count;
    }
    nk_clear(&ctx->ctx);
}

struct nk_context *mt_nuklear_get_context(MtNuklearContext *ctx)
{
    return &ctx->ctx;
}

void mt_nuklear_destroy(MtNuklearContext *ctx)
{
    mt_render.destroy_sampler(ctx->engine->device, ctx->sampler);
    mt_render.destroy_image(ctx->engine->device, ctx->font_image);
    nk_buffer_free(&ctx->cmds);
    nk_font_atlas_clear(&ctx->atlas);
    nk_free(&ctx->ctx);
    mt_free(ctx->engine->alloc, ctx);
}
