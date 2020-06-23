#pragma once

#include "api_types.h"
#include <motor/base/hashmap.h>
#include <motor/base/threads.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct MtEngine MtEngine;
typedef struct MtAllocator MtAllocator;
typedef struct MtAssetManager MtAssetManager;

typedef struct MtAssetVT MtAssetVT;

typedef struct MtAsset
{
    MtAssetVT *vt;
    const char *path;
} MtAsset;

typedef struct MtAssetVT
{
    const char *name;

    const char **extensions;
    uint32_t extension_count;

    uint32_t size;

    bool (*init)(MtAssetManager *, MtAsset *, const char *path);
    void (*destroy)(MtAsset *);
} MtAssetVT;

typedef struct MtAssetManager
{
    MtEngine *engine;
    MtAllocator *alloc;
    MtAssetManager *parent;

    /*array*/ MtAssetVT **asset_types;
    MtHashMap asset_type_map;

    /*array*/ MtAsset **assets;
    MtHashMap asset_map;

    MtMutex mutex;
} MtAssetManager;

MT_ENGINE_API void
mt_asset_manager_init(MtAssetManager *am, MtAssetManager *parent, MtEngine *engine);

MT_ENGINE_API MtAsset *mt_asset_manager_load(MtAssetManager *am, const char *path);

// Loads the asset through the engine's thread pool and
// stores the asset in *out_asset when the load is complete
MT_ENGINE_API void
mt_asset_manager_queue_load(MtAssetManager *am, const char *path, MtAsset **out_asset);

MT_ENGINE_API MtAsset *mt_asset_manager_get(MtAssetManager *am, const char *path);

MT_ENGINE_API void
mt_asset_manager_get_all(MtAssetManager *am, MtAssetVT *type, uint32_t *count, MtAsset **assets);

MT_ENGINE_API void mt_asset_manager_destroy(MtAssetManager *am);

#ifdef __cplusplus
}
#endif
