#include <motor/engine/picker.h>

#include <motor/base/allocator.h>
#include <motor/graphics/window.h>
#include <motor/graphics/renderer.h>
#include <motor/engine/engine.h>
#include <motor/engine/nuklear.h>
#include <motor/engine/nuklear_impl.h>
#include <motor/engine/camera.h>

#include "pipeline_utils.inl"

#include <assert.h>
#include <stdio.h>

static_assert(sizeof(MtPickerSelectionType) == sizeof(uint32_t), "");

struct MtPicker
{
    MtEngine *engine;
    MtRenderGraph *graph;
    MtPipeline *picking_pipeline;

    // Temporary data
    int32_t x;
    int32_t y;
    MtCameraUniform camera_uniform;
    MtPickerDrawCallback draw;
    void *user_data;
};

static void picking_pass_builder(MtRenderGraph *graph, MtCmdBuffer *cb, void *user_data)
{
    MtPicker *picker = user_data;

    // Draw models
    mt_render.cmd_bind_pipeline(cb, picker->picking_pipeline);
    mt_render.cmd_bind_uniform(cb, &picker->camera_uniform, sizeof(picker->camera_uniform), 0, 0);
    picker->draw(cb, picker->user_data);
}

static void picking_transfer_pass_builder(MtRenderGraph *graph, MtCmdBuffer *cb, void *user_data)
{
    MtPicker *picker = user_data;

    struct nk_context *nk = mt_nuklear_get_context(picker->engine->nk_ctx);

    MtBuffer *picking_buffer = mt_render.graph_get_buffer(picker->graph, "picking_buffer");

    if (picker->x > 0 && picker->y > 0 && !nk_window_is_any_hovered(nk))
    {
        mt_render.cmd_copy_image_to_buffer(
            cb,
            &(MtImageCopyView){
                .image = mt_render.graph_get_image(graph, "picking"),
                .offset = {picker->x, picker->y, 0},
            },
            &(MtBufferCopyView){.buffer = picking_buffer},
            (MtExtent3D){1, 1, 1});
    }
}

static bool picking_clear_color(uint32_t index, MtClearColorValue *color)
{
    if (color)
    {
        color->uint32[0] = UINT32_MAX;
        color->uint32[1] = UINT32_MAX;
        color->uint32[2] = UINT32_MAX;
        color->uint32[3] = UINT32_MAX;
    }
    return true;
}

static void picking_graph_builder(MtRenderGraph *graph, void *user_data)
{
    MtPicker *picker = user_data;

    uint32_t width, height;
    mt_window.get_size(picker->engine->window, &width, &height);

    mt_render.graph_add_image(
        graph,
        "depth",
        &(MtImageCreateInfo){
            .width = width,
            .height = height,
            .format = MT_FORMAT_D32_SFLOAT,
        });

    mt_render.graph_add_image(
        graph,
        "picking",
        &(MtImageCreateInfo){
            .width = width,
            .height = height,
            .format = MT_FORMAT_R32_UINT,
        });

    mt_render.graph_add_buffer(
        graph,
        "picking_buffer",
        &(MtBufferCreateInfo){
            .usage = MT_BUFFER_USAGE_TRANSFER,
            .memory = MT_BUFFER_MEMORY_HOST,
            .size = sizeof(uint32_t),
        });

    {
        MtRenderGraphPass *picking_pass =
            mt_render.graph_add_pass(graph, "picking_pass", MT_PIPELINE_STAGE_ALL_GRAPHICS);
        mt_render.pass_write(picking_pass, MT_PASS_WRITE_COLOR_ATTACHMENT, "picking");
        mt_render.pass_write(picking_pass, MT_PASS_WRITE_DEPTH_STENCIL_ATTACHMENT, "depth");
        mt_render.pass_set_color_clearer(picking_pass, picking_clear_color);
        mt_render.pass_set_builder(picking_pass, picking_pass_builder);
    }

    {
        MtRenderGraphPass *picking_transfer_pass =
            mt_render.graph_add_pass(graph, "picking_transfer_pass", MT_PIPELINE_STAGE_TRANSFER);
        mt_render.pass_read(picking_transfer_pass, MT_PASS_READ_IMAGE_TRANSFER, "picking");
        mt_render.pass_write(picking_transfer_pass, MT_PASS_WRITE_BUFFER, "picking_buffer");
        mt_render.pass_set_builder(picking_transfer_pass, picking_transfer_pass_builder);
    }
}

MtPicker *mt_picker_create(MtEngine *engine)
{
    MtPicker *picker = mt_alloc(engine->alloc, sizeof(*picker));
    memset(picker, 0, sizeof(*picker));

    picker->engine = engine;

    const char *path = "../shaders/picking.hlsl";

    FILE *f = fopen(path, "rb");
    if (!f)
    {
        mt_log_error("Failed to open file: %s", path);
        return NULL;
    }

    fseek(f, 0, SEEK_END);
    size_t input_size = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *input = mt_alloc(engine->alloc, input_size);
    fread(input, input_size, 1, f);
    fclose(f);

    picker->picking_pipeline = create_pipeline(engine, path, input, input_size);

    picker->graph = mt_render.create_graph(engine->device, NULL, picker);
    mt_render.graph_set_builder(picker->graph, picking_graph_builder);
    mt_render.graph_bake(picker->graph);

    return picker;
}

void mt_picker_destroy(MtPicker *picker)
{
    MtAllocator *alloc = picker->engine->alloc;

    mt_render.destroy_graph(picker->graph);
    mt_render.destroy_pipeline(picker->engine->device, picker->picking_pipeline);

    mt_free(alloc, picker);
}

MT_ENGINE_API void mt_picker_resize(MtPicker *picker)
{
    mt_render.graph_on_resize(picker->graph);
}

uint32_t mt_picker_pick(
    MtPicker *picker,
    const MtCameraUniform *camera_uniform,
    int32_t x,
    int32_t y,
    MtPickerDrawCallback draw,
    void *user_data)
{
    picker->x = x;
    picker->y = y;
    picker->camera_uniform = *camera_uniform;
    picker->draw = draw;
    picker->user_data = user_data;

    mt_render.graph_execute(picker->graph);
    mt_render.graph_wait_all(picker->graph);

    MtBuffer *picking_buffer = mt_render.graph_get_buffer(picker->graph, "picking_buffer");

    void *mapping = mt_render.map_buffer(picker->engine->device, picking_buffer);
    uint32_t value = UINT32_MAX;
    memcpy(&value, mapping, sizeof(uint32_t));
    memcpy(mapping, &(uint32_t){MT_PICKER_SELECTION_DESELECT}, sizeof(uint32_t));
    mt_render.unmap_buffer(picker->engine->device, picking_buffer);

    return value;
}
