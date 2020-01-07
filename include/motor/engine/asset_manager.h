#pragma once

#include <motor/base/hashmap.h>
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
} MtAssetManager;

void mt_asset_manager_init(MtAssetManager *am, MtEngine *engine);

MtAsset *mt_asset_manager_load(MtAssetManager *am, const char *path);

void mt_asset_manager_destroy(MtAssetManager *am);
