#include <motor/engine/engine.h>

#include <motor/base/arena.h>
#include <motor/base/allocator.h>
#include <motor/graphics/renderer.h>
#include <motor/graphics/vulkan/vulkan_device.h>
#include <motor/graphics/vulkan/glfw_window.h>
#include <shaderc/shaderc.h>
#include <string.h>

void mt_engine_init(MtEngine *engine)
{
    memset(engine, 0, sizeof(*engine));
#if 0
    engine->alloc = mt_alloc(NULL, sizeof(MtAllocator));
    mt_arena_init(engine->alloc, 1 << 16);
#endif

    mt_glfw_vulkan_init(&engine->window_system);

    engine->device = mt_vulkan_device_init(
        &(MtVulkanDeviceCreateInfo){.window_system = &engine->window_system}, engine->alloc);

    mt_glfw_vulkan_window_init(&engine->window, engine->device, 800, 600, "Hello", engine->alloc);

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
            .anisotropy = true,
            .mag_filter = MT_FILTER_LINEAR,
            .min_filter = MT_FILTER_LINEAR,
        });

    mt_asset_manager_init(&engine->asset_manager, engine);
}

void mt_engine_destroy(MtEngine *engine)
{
    mt_asset_manager_destroy(&engine->asset_manager);

    mt_render.destroy_image(engine->device, engine->default_cubemap);
    mt_render.destroy_image(engine->device, engine->white_image);
    mt_render.destroy_image(engine->device, engine->black_image);
    mt_render.destroy_sampler(engine->device, engine->default_sampler);

    shaderc_compiler_release(engine->compiler);

    engine->window.vt->destroy(engine->window.inst);
    mt_render.destroy_device(engine->device);
    engine->window_system.vt->destroy();

#if 0
    mt_arena_destroy(engine->alloc);
#endif
}
