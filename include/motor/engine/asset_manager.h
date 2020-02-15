#pragma once

#include "api_types.h"
#include "asset.h"
#include <motor/base/hashmap.h>
#include <motor/base/threads.h>

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

MT_ENGINE_API void mt_asset_manager_init(MtAssetManager *am, MtEngine *engine);

MT_ENGINE_API MtAsset *mt_asset_manager_load(MtAssetManager *am, const char *path);

// Loads the asset through the engine's thread pool and
// stores the asset in *out_asset when the load is complete
MT_ENGINE_API void
mt_asset_manager_queue_load(MtAssetManager *am, const char *path, MtAsset **out_asset);

MT_ENGINE_API MtAsset *mt_asset_manager_get(MtAssetManager *am, const char *path);

MT_ENGINE_API void mt_asset_manager_destroy(MtAssetManager *am);
