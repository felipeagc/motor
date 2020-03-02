#include <motor/engine/picker.h>

#include <motor/base/log.h>
#include <motor/base/allocator.h>
#include <motor/graphics/window.h>
#include <motor/graphics/renderer.h>
#include <motor/engine/engine.h>
#include <motor/engine/camera.h>
#include <motor/engine/assets/pipeline_asset.h>

#include <assert.h>
#include <stdio.h>

struct MtPicker
{
    MtEngine *engine;
    MtRenderGraph *graph;

    // Temporary data
    int32_t x;
    int32_t y;
    MtCameraUniform camera_uniform;
    MtPickerDrawCallback draw;
    void *user_data;
};

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
    mt_render.graph_add_image(
        graph,
        "depth",
        &(MtRenderGraphImage){
            .size_class = MT_SIZE_CLASS_SWAPCHAIN_RELATIVE,
            .width = 1.0f,
            .height = 1.0f,
            .format = MT_FORMAT_D32_SFLOAT,
        });

    mt_render.graph_add_image(
        graph,
        "picking",
        &(MtRenderGraphImage){
            .size_class = MT_SIZE_CLASS_SWAPCHAIN_RELATIVE,
            .width = 1.0f,
            .height = 1.0f,
            .format = MT_FORMAT_R32_UINT,
        });

    mt_render.graph_add_buffer(
        graph,
        "picking_buffer",
        &(MtBufferCreateInfo){
            .usage = MT_BUFFER_USAGE_STORAGE,
            .memory = MT_BUFFER_MEMORY_HOST,
            .size = sizeof(uint32_t),
        });

    {
        MtRenderGraphPass *picking_pass =
            mt_render.graph_add_pass(graph, "picking_pass", MT_PIPELINE_STAGE_ALL_GRAPHICS);
        mt_render.pass_write(picking_pass, MT_PASS_WRITE_COLOR_ATTACHMENT, "picking");
        mt_render.pass_write(picking_pass, MT_PASS_WRITE_DEPTH_STENCIL_ATTACHMENT, "depth");
        mt_render.pass_set_color_clearer(picking_pass, picking_clear_color);
    }

    {
        MtRenderGraphPass *picking_transfer_pass =
            mt_render.graph_add_pass(graph, "picking_transfer_pass", MT_PIPELINE_STAGE_COMPUTE);
        mt_render.pass_read(picking_transfer_pass, MT_PASS_READ_IMAGE_SAMPLED, "picking");
        mt_render.pass_write(picking_transfer_pass, MT_PASS_WRITE_BUFFER, "picking_buffer");
    }
}

MtPicker *mt_picker_create(MtEngine *engine)
{
    MtPicker *picker = mt_alloc(engine->alloc, sizeof(*picker));
    memset(picker, 0, sizeof(*picker));

    picker->engine = engine;

    picker->graph = mt_render.create_graph(engine->device, engine->swapchain, false);
    mt_render.graph_set_user_data(picker->graph, picker);
    mt_render.graph_set_builder(picker->graph, picking_graph_builder);

    return picker;
}

void mt_picker_destroy(MtPicker *picker)
{
    MtAllocator *alloc = picker->engine->alloc;

    mt_render.destroy_graph(picker->graph);

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

    {
        MtCmdBuffer *cb = mt_render.pass_begin(picker->graph, "picking_pass");

        // Draw models
        mt_render.cmd_bind_pipeline(cb, picker->engine->picking_pipeline->pipeline);
        mt_render.cmd_bind_uniform(
            cb, &picker->camera_uniform, sizeof(picker->camera_uniform), 0, 0);
        picker->draw(cb, picker->user_data);

        mt_render.pass_end(picker->graph, "picking_pass");
    }

    {
        MtCmdBuffer *cb = mt_render.pass_begin(picker->graph, "picking_transfer_pass");

        if (picker->x > 0 && picker->y > 0)
        {
            MtBuffer *picking_buffer = mt_render.graph_get_buffer(picker->graph, "picking_buffer");

            struct
            {
                int32_t x;
                int32_t y;
            } coords;
            coords.x = picker->x;
            coords.y = picker->y;

            mt_render.cmd_bind_pipeline(cb, picker->engine->picking_transfer_pipeline->pipeline);
            mt_render.cmd_bind_storage_buffer(cb, picking_buffer, 0, 0);
            mt_render.cmd_bind_uniform(cb, &coords, sizeof(coords), 0, 1);
            mt_render.cmd_bind_image(cb, mt_render.graph_get_image(picker->graph, "picking"), 0, 2);
            mt_render.cmd_dispatch(cb, 1, 1, 1);
        }

        mt_render.pass_end(picker->graph, "picking_transfer_pass");
    }

    mt_render.graph_execute(picker->graph);
    mt_render.graph_wait_all(picker->graph);

    MtBuffer *picking_buffer = mt_render.graph_get_buffer(picker->graph, "picking_buffer");

    void *mapping = mt_render.map_buffer(picker->engine->device, picking_buffer);
    uint32_t value = UINT32_MAX;
    memcpy(&value, mapping, sizeof(uint32_t));
    memcpy(mapping, &(uint32_t){UINT32_MAX}, sizeof(uint32_t));
    mt_render.unmap_buffer(picker->engine->device, picking_buffer);

    return value;
}
