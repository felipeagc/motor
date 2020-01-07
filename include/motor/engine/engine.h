#pragma once

#include "asset_manager.h"
#include <motor/graphics/window.h>

typedef struct MtDevice MtDevice;
typedef struct MtImage MtImage;
typedef struct MtSampler MtSampler;
typedef struct shaderc_compiler *shaderc_compiler_t;

typedef struct MtEngine
{
    MtDevice *device;
    MtIWindowSystem window_system;
    MtIWindow window;

    MtAllocator *alloc;
    MtAssetManager asset_manager;

    shaderc_compiler_t compiler;

    MtImage *white_image;
    MtImage *black_image;

    MtSampler *default_sampler;
} MtEngine;

void mt_engine_init(MtEngine *engine);

void mt_engine_destroy(MtEngine *engine);
