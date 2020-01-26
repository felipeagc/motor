#include <motor/engine/engine.h>

#include <motor/base/arena.h>
#include <motor/base/allocator.h>
#include <motor/graphics/window.h>
#include <motor/graphics/renderer.h>
#include <motor/graphics/vulkan/vulkan_device.h>
#include <motor/graphics/vulkan/glfw_window.h>
#include <motor/engine/file_watcher.h>
#include <motor/engine/ui.h>
#include <shaderc/shaderc.h>
#include <string.h>
#include <stdio.h>

void asset_watcher_handler(MtFileWatcherEvent *e, void *user_data)
{
    MtEngine *engine = (MtEngine *)user_data;

    switch (e->type)
    {
        case MT_FILE_WATCHER_EVENT_MODIFY:
        {
            mt_asset_manager_load(&engine->asset_manager, e->src);
            break;
        }
        default: break;
    }
}

void mt_engine_init(MtEngine *engine, uint32_t num_threads)
{
    memset(engine, 0, sizeof(*engine));
#if 0
    engine->alloc = mt_alloc(NULL, sizeof(MtAllocator));
    mt_arena_init(engine->alloc, 1 << 16);
#endif

    printf("Using %u threads\n", num_threads);

    mt_glfw_vulkan_window_system_init();

    engine->device = mt_vulkan_device_init(
        &(MtVulkanDeviceCreateInfo){
            .num_threads = num_threads,
        },
        engine->alloc);

    engine->window    = mt_window.create(1280, 720, "Motor", engine->alloc);
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
                .format      = MT_FORMAT_RGBA16_SFLOAT,
                .width       = 1,
                .height      = 1,
                .layer_count = 6,
            });

        for (uint32_t i = 0; i < 6; i++)
        {
            mt_render.transfer_to_image(
                engine->device,
                &(MtImageCopyView){
                    .image       = engine->default_cubemap,
                    .array_layer = i,
                },
                8,
                (uint8_t[8]){0, 0, 0, 0, 0, 0, 0, 0});
        }
    }

    engine->default_sampler = mt_render.create_sampler(
        engine->device,
        &(MtSamplerCreateInfo){
            .anisotropy   = true,
            .mag_filter   = MT_FILTER_LINEAR,
            .min_filter   = MT_FILTER_LINEAR,
            .address_mode = MT_SAMPLER_ADDRESS_MODE_REPEAT,
            .border_color = MT_BORDER_COLOR_FLOAT_OPAQUE_WHITE,
        });

    mt_thread_pool_init(&engine->thread_pool, num_threads, engine->alloc);
    mt_asset_manager_init(&engine->asset_manager, engine);
    mt_entity_manager_init(&engine->entity_manager, engine->alloc);

    engine->ui      = mt_ui_create(engine->alloc, engine->window, &engine->asset_manager);
    engine->watcher = mt_file_watcher_create(
        engine->alloc, MT_FILE_WATCHER_EVENT_MODIFY, asset_watcher_handler, "../assets");
}

void mt_engine_destroy(MtEngine *engine)
{
    mt_entity_manager_destroy(&engine->entity_manager);
    mt_asset_manager_destroy(&engine->asset_manager);
    mt_thread_pool_destroy(&engine->thread_pool);

    mt_file_watcher_destroy(engine->watcher);
    mt_ui_destroy(engine->ui);

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
