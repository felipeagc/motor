#pragma once

#include "asset.h"

typedef struct MtEngine MtEngine;
typedef struct MtAllocator MtAllocator;

typedef struct MtAssetManager {
    MtEngine *engine;
    MtAllocator *alloc;
    /*array*/ MtIAsset *assets;
} MtAssetManager;

void mt_asset_manager_init(MtAssetManager *am, MtEngine *engine);

MtIAsset *mt_asset_manager_add(MtAssetManager *am, MtIAsset asset);

void mt_asset_manager_destroy(MtAssetManager *am);
