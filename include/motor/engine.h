#pragma once

#include "asset_manager.h"
#include "window.h"
#include "allocator.h"

typedef struct MtDevice MtDevice;

typedef struct MtEngine {
    MtDevice *device;
    MtIWindowSystem window_system;
    MtIWindow window;

    MtAllocator alloc;
    MtAssetManager asset_manager;
} MtEngine;

void mt_engine_init(MtEngine *engine);

void mt_engine_destroy(MtEngine *engine);
