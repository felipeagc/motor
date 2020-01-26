#pragma once

#include <motor/base/hashmap.h>
#include <motor/base/threads.h>
#include "asset.h"

typedef struct MtEngine MtEngine;
typedef struct MtAllocator MtAllocator;

typedef struct MtAssetManager
{
    MtEngine *engine;
    MtAllocator *alloc;
    /*array*/ MtAssetVT **asset_types;

    /*array*/ MtIAsset *assets;
    MtHashMap asset_map;

    MtMutex mutex;
} MtAssetManager;

void mt_asset_manager_init(MtAssetManager *am, MtEngine *engine);

MtAsset *mt_asset_manager_load(MtAssetManager *am, const char *path);

// Loads the asset through the engine's thread pool and
// stores the asset in *out_asset when the load is complete
void mt_asset_manager_queue_load(MtAssetManager *am, const char *path, MtAsset **out_asset);

MtAsset *mt_asset_manager_get(MtAssetManager *am, const char *path);

void mt_asset_manager_destroy(MtAssetManager *am);
