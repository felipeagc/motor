#include <motor/engine/engine.h>

#include <motor/base/log.h>
#include <motor/base/arena.h>
#include <motor/base/allocator.h>
#include <motor/graphics/window.h>
#include <motor/graphics/renderer.h>
#include <motor/graphics/vulkan/vulkan_device.h>
#include <motor/graphics/vulkan/glfw_window.h>
#include <motor/engine/file_watcher.h>
#include <motor/engine/physics.h>
#include <motor/engine/picker.h>
#include <motor/engine/asset_manager.h>
#include <motor/engine/imgui_impl.h>
#include <motor/engine/meshes.h>
#include <shaderc/shaderc.h>
#include <string.h>
#include <stdio.h>

void asset_watcher_handler(MtFileWatcherEvent *e, void *user_data)
{
    MtEngine *engine = (MtEngine *)user_data;

    switch (e->type)
    {
        case MT_FILE_WATCHER_EVENT_MODIFY: {
            mt_asset_manager_load(engine->asset_manager, e->src);
            break;
        }
        default: break;
    }
}

void mt_engine_init(MtEngine *engine)
{
    memset(engine, 0, sizeof(*engine));
#if 0
    engine->alloc = mt_alloc(NULL, sizeof(MtAllocator));
    mt_arena_init(engine->alloc, 1 << 16);
#endif

    mt_log_init();

    uint32_t num_threads = mt_cpu_count() / 2;

    mt_log_debug("Using %u threads", num_threads);

    mt_glfw_vulkan_window_system_init();

    engine->device = mt_vulkan_device_init(
        &(MtVulkanDeviceCreateInfo){
            .num_threads = num_threads,
        },
        engine->alloc);

    engine->window = mt_window.create(1280, 720, "Motor", engine->alloc);
    engine->swapchain = mt_render.create_swapchain(engine->device, engine->window, engine->alloc);

    engine->compiler = shaderc_compiler_initialize();

    {
        engine->white_image = mt_render.create_image(
            engine->device,
            &(MtImageCreateInfo){.format = MT_FORMAT_RGBA8_UNORM, .width = 1, .height = 1});

        mt_render.transfer_to_image(
            engine->device,
            &(MtImageCopyView){.image = engine->white_image},
            4,
            (uint8_t[]){255, 255, 255, 255});
    }

    {
        engine->black_image = mt_render.create_image(
            engine->device,
            &(MtImageCreateInfo){.format = MT_FORMAT_RGBA8_UNORM, .width = 1, .height = 1});

        mt_render.transfer_to_image(
            engine->device,
            &(MtImageCopyView){.image = engine->black_image},
            4,
            (uint8_t[4]){0, 0, 0, 255});
    }

    {
        engine->default_cubemap = mt_render.create_image(
            engine->device,
            &(MtImageCreateInfo){
                .format = MT_FORMAT_RGBA16_SFLOAT,
                .width = 1,
                .height = 1,
                .layer_count = 6,
            });

        for (uint32_t i = 0; i < 6; i++)
        {
            mt_render.transfer_to_image(
                engine->device,
                &(MtImageCopyView){
                    .image = engine->default_cubemap,
                    .array_layer = i,
                },
                8,
                (uint8_t[8]){0, 0, 0, 0, 0, 0, 0, 0});
        }
    }

    engine->default_sampler = mt_render.create_sampler(
        engine->device,
        &(MtSamplerCreateInfo){
            .anisotropy = true,
            .mag_filter = MT_FILTER_LINEAR,
            .min_filter = MT_FILTER_LINEAR,
            .address_mode = MT_SAMPLER_ADDRESS_MODE_REPEAT,
            .border_color = MT_BORDER_COLOR_FLOAT_OPAQUE_WHITE,
        });

    mt_mesh_init_sphere(&engine->sphere_mesh, engine);

    mt_thread_pool_init(&engine->thread_pool, num_threads, engine->alloc);

    engine->watcher = mt_file_watcher_create(
        engine->alloc, MT_FILE_WATCHER_EVENT_MODIFY, asset_watcher_handler, "../assets");

    engine->physics = mt_physics_create(engine->alloc);
    engine->picker = mt_picker_create(engine);

    engine->asset_manager = mt_alloc(engine->alloc, sizeof(*engine->asset_manager));
    mt_asset_manager_init(engine->asset_manager, engine);

    MtAssetManager *am = engine->asset_manager;
    mt_asset_manager_queue_load(am, "../shaders/pbr.hlsl", (MtAsset **)&engine->pbr_pipeline);
    mt_asset_manager_queue_load(am, "../shaders/gizmo.hlsl", (MtAsset **)&engine->gizmo_pipeline);
    mt_asset_manager_queue_load(
        am, "../shaders/wireframe.hlsl", (MtAsset **)&engine->wireframe_pipeline);
    mt_asset_manager_queue_load(am, "../shaders/skybox.hlsl", (MtAsset **)&engine->skybox_pipeline);
    mt_asset_manager_queue_load(am, "../shaders/brdf.hlsl", (MtAsset **)&engine->brdf_pipeline);
    mt_asset_manager_queue_load(
        am, "../shaders/irradiance_cube.hlsl", (MtAsset **)&engine->irradiance_pipeline);
    mt_asset_manager_queue_load(
        am, "../shaders/prefilter_env_map.hlsl", (MtAsset **)&engine->prefilter_env_pipeline);
    mt_asset_manager_queue_load(am, "../shaders/imgui.hlsl", (MtAsset **)&engine->imgui_pipeline);
    mt_asset_manager_queue_load(
        am, "../shaders/picking.hlsl", (MtAsset **)&engine->picking_pipeline);
    mt_asset_manager_queue_load(
        am, "../shaders/picking_transfer.hlsl", (MtAsset **)&engine->picking_transfer_pipeline);

    mt_thread_pool_wait_all(&engine->thread_pool);

    engine->imgui_ctx = mt_imgui_create(engine);
}

void mt_engine_destroy(MtEngine *engine)
{
    if (engine->current_scene.inst)
    {
        engine->current_scene.vt->destroy(engine->current_scene.inst);
    }

    mt_picker_destroy(engine->picker);
    mt_physics_destroy(engine->physics);

    mt_asset_manager_destroy(engine->asset_manager);
    mt_free(engine->alloc, engine->asset_manager);

    mt_thread_pool_destroy(&engine->thread_pool);

    mt_file_watcher_destroy(engine->watcher);
    mt_imgui_destroy(engine->imgui_ctx);

    mt_mesh_destroy(&engine->sphere_mesh);
    mt_render.destroy_image(engine->device, engine->default_cubemap);
    mt_render.destroy_image(engine->device, engine->white_image);
    mt_render.destroy_image(engine->device, engine->black_image);
    mt_render.destroy_sampler(engine->device, engine->default_sampler);

    shaderc_compiler_release(engine->compiler);

    mt_render.destroy_swapchain(engine->swapchain);
    mt_window.destroy(engine->window);

    mt_render.destroy_device(engine->device);
    mt_window.destroy_window_system();

#if 0
    mt_arena_destroy(engine->alloc);
#endif
}

void mt_engine_set_scene(MtEngine *engine, const MtIScene *scene)
{
    if (engine->current_scene.inst)
    {
        engine->current_scene.vt->destroy(engine->current_scene.inst);
    }

    if (scene)
    {
        engine->current_scene = *scene;

        if (engine->current_scene.vt->init)
        {
            engine->current_scene.vt->init(engine->current_scene.inst);
        }
    }
    else
    {
        engine->current_scene.inst = NULL;
    }
}

void mt_engine_update(MtEngine *engine)
{
    MtScene *scene = engine->current_scene.inst;

    mt_file_watcher_poll(engine->watcher, engine);
    mt_window.poll_events();

    MtEvent event;
    while (mt_window.next_event(engine->window, &event))
    {
        mt_imgui_handle_event(engine->imgui_ctx, &event);

        if (event.type == MT_EVENT_FRAMEBUFFER_RESIZED)
        {
            mt_picker_resize(engine->picker);
        }

        if (scene)
        {
            mt_perspective_camera_on_event(&scene->cam, &event);

            if (event.type == MT_EVENT_FRAMEBUFFER_RESIZED)
            {
                mt_render.graph_on_resize(scene->graph);
            }

            engine->current_scene.vt->on_event(scene, &event);
        }
    }

    if (scene)
    {
        float delta_time = mt_render.swapchain_get_delta_time(engine->swapchain);

        // Update cam
        uint32_t width, height;
        mt_window.get_size(engine->window, &width, &height);
        float aspect = (float)width / (float)height;
        mt_perspective_camera_update(&scene->cam, engine->window, aspect, delta_time);

        mt_imgui_begin(engine->imgui_ctx);
        igNewFrame();

        if (engine->current_scene.vt->draw_ui)
        {
            engine->current_scene.vt->draw_ui(scene, delta_time);
        }

        igRender();

        if (engine->current_scene.vt->update)
        {
            engine->current_scene.vt->update(scene, delta_time);
        }

        mt_render.graph_execute(scene->graph);
    }
}
