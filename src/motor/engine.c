#include "../../include/motor/engine.h"

#include "../../include/motor/arena.h"
#include "../../include/motor/allocator.h"
#include "../../include/motor/renderer.h"
#include "../../include/motor/vulkan/vulkan_device.h"
#include "../../include/motor/vulkan/glfw_window.h"
#include <shaderc/shaderc.h>
#include <string.h>

void mt_engine_init(MtEngine *engine) {
    memset(engine, 0, sizeof(*engine));
#if 0
    engine->alloc = mt_alloc(NULL, sizeof(MtAllocator));
    mt_arena_init(engine->alloc, 1 << 16);
#endif

    mt_glfw_vulkan_init(&engine->window_system);

    engine->device = mt_vulkan_device_init(
        &(MtVulkanDeviceCreateInfo){.window_system = &engine->window_system},
        engine->alloc);

    mt_glfw_vulkan_window_init(
        &engine->window, engine->device, 800, 600, "Hello", engine->alloc);

    engine->compiler = shaderc_compiler_initialize();

    mt_asset_manager_init(&engine->asset_manager, engine);
}

void mt_engine_destroy(MtEngine *engine) {
    mt_asset_manager_destroy(&engine->asset_manager);

    shaderc_compiler_release(engine->compiler);

    engine->window.vt->destroy(engine->window.inst);
    mt_render.destroy_device(engine->device);
    engine->window_system.vt->destroy();

#if 0
    mt_arena_destroy(engine->alloc);
#endif
}
