#pragma once

#include "asset_manager.h"
#include "window.h"

typedef struct MtDevice MtDevice;
typedef struct shaderc_compiler *shaderc_compiler_t;

typedef struct MtEngine {
    MtDevice *device;
    MtIWindowSystem window_system;
    MtIWindow window;

    MtAllocator *alloc;
    MtAssetManager asset_manager;

    shaderc_compiler_t compiler;
} MtEngine;

void mt_engine_init(MtEngine *engine);

void mt_engine_destroy(MtEngine *engine);
