#pragma once

#include "asset_manager.h"
#include "entities.h"
#include <motor/base/thread_pool.h>

typedef struct MtDevice MtDevice;
typedef struct MtWindow MtWindow;
typedef struct MtSwapchain MtSwapchain;
typedef struct MtImage MtImage;
typedef struct MtSampler MtSampler;
typedef struct MtUIRenderer MtUIRenderer;
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

    MtUIRenderer *ui;
    MtFileWatcher *watcher;

    shaderc_compiler_t compiler;

    MtImage *white_image;
    MtImage *black_image;
    MtImage *default_cubemap;
    MtSampler *default_sampler;
} MtEngine;

void mt_engine_init(MtEngine *engine, uint32_t num_threads);

void mt_engine_destroy(MtEngine *engine);
