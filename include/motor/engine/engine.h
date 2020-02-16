#pragma once

#include "api_types.h"
#include "asset_manager.h"
#include "entities.h"
#include "nuklear_impl.h"
#include <motor/base/thread_pool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct MtDevice MtDevice;
typedef struct MtWindow MtWindow;
typedef struct MtSwapchain MtSwapchain;
typedef struct MtImage MtImage;
typedef struct MtSampler MtSampler;
typedef struct MtFileWatcher MtFileWatcher;
typedef struct shaderc_compiler *shaderc_compiler_t;

typedef struct MtEngine
{
    MtDevice *device;
    MtWindow *window;
    MtSwapchain *swapchain;

    MtAllocator *alloc;
    MtThreadPool thread_pool;
    MtAssetManager asset_manager;
    MtEntityManager entity_manager;

    MtNuklearContext *nk_ctx;
    MtFileWatcher *watcher;

    shaderc_compiler_t compiler;

    MtImage *white_image;
    MtImage *black_image;
    MtImage *default_cubemap;
    MtSampler *default_sampler;
} MtEngine;

MT_ENGINE_API void mt_engine_init(MtEngine *engine);

MT_ENGINE_API void mt_engine_destroy(MtEngine *engine);

#ifdef __cplusplus
}
#endif
