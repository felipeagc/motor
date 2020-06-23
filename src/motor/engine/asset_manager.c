#include <motor/engine/asset_manager.h>

#include <stdio.h>
#include <string.h>

#include <motor/base/api_types.h>
#include <motor/base/filesystem.h>
#include <motor/base/log.h>
#include <motor/base/allocator.h>
#include <motor/base/array.h>
#include <motor/base/thread_pool.h>

#include <motor/graphics/renderer.h>

#include <motor/engine/engine.h>
#include <motor/engine/assets/image_asset.h>
#include <motor/engine/assets/pipeline_asset.h>
#include <motor/engine/assets/font_asset.h>
#include <motor/engine/assets/gltf_asset.h>

static void register_asset_type(MtAssetManager *am, MtAssetVT *vt)
{
    mt_array_push(am->alloc, am->asset_types, vt);
    for (uint32_t i = 0; i < vt->extension_count; ++i)
    {
        mt_hash_set_ptr(&am->asset_type_map, mt_hash_str(vt->extensions[i]), vt);
    }
}

void mt_asset_manager_init(MtAssetManager *am, MtAssetManager *parent, MtEngine *engine)
{
    memset(am, 0, sizeof(*am));
    am->engine = engine;
    am->alloc = engine->alloc;
    am->parent = parent;

    mt_mutex_init(&am->mutex);

    mt_hash_init(&am->asset_type_map, 51, am->alloc);
    mt_hash_init(&am->asset_map, 51, am->alloc);

    register_asset_type(am, mt_image_asset_vt);
    register_asset_type(am, mt_pipeline_asset_vt);
    register_asset_type(am, mt_font_asset_vt);
    register_asset_type(am, mt_gltf_asset_vt);
}

MtAsset *mt_asset_manager_load(MtAssetManager *am, const char *path)
{
    mt_render.set_thread_id(mt_thread_pool_get_task_id());

    const char *path_ext = mt_path_ext(path);

    MtAssetVT *vt = mt_hash_get_ptr(&am->asset_type_map, mt_hash_str(path_ext));
    if (!vt)
    {
        mt_log_error("No asset loader found for file: %s", path);
        return NULL;
    }

    mt_log_debug("Loading asset: %s", path);

    MtAsset *temp_asset_ptr = mt_alloc(am->alloc, vt->size);
    memset(temp_asset_ptr, 0, vt->size);
    temp_asset_ptr->path = mt_strdup(am->alloc, path);
    temp_asset_ptr->vt = vt;

    bool initialized = vt->init(am, temp_asset_ptr, path);

    temp_asset_ptr->path = mt_strdup(am->alloc, path);
    temp_asset_ptr->vt = vt;

    if (!initialized)
    {
        mt_log_error("Failed to load asset: %s", path);
        mt_free(am->alloc, temp_asset_ptr);
        temp_asset_ptr = NULL;
    }

    uint64_t path_hash = mt_hash_str(path);

    mt_mutex_lock(&am->mutex);
    MtAsset *existing = mt_hash_get_ptr(&am->asset_map, path_hash);
    mt_mutex_unlock(&am->mutex);

    if (existing)
    {
        if (temp_asset_ptr)
        {
            mt_mutex_lock(&am->mutex);

            existing->vt->destroy(existing);
            memcpy(existing, temp_asset_ptr, vt->size);
            mt_free(am->alloc, temp_asset_ptr);

            mt_mutex_unlock(&am->mutex);
        }

        return existing;
    }
    else
    {
        if (temp_asset_ptr)
        {
            mt_mutex_lock(&am->mutex);

            mt_array_push(am->alloc, am->assets, temp_asset_ptr);
            MtAsset *asset_ptr = *mt_array_last(am->assets);
            mt_hash_set_ptr(&am->asset_map, path_hash, asset_ptr);

            mt_mutex_unlock(&am->mutex);
            return asset_ptr;
        }

        mt_log_error("Failed to load asset: %s", path);
        return NULL;
    }

    return NULL;
}

typedef struct AssetLoadInfo
{
    MtAssetManager *asset_manager;
    MtAsset **out_asset;
    const char *path;
} AssetLoadInfo;

static int32_t asset_load(void *arg)
{
    AssetLoadInfo *info = arg;
    MtAssetManager *am = info->asset_manager;

    MtAsset *asset = mt_asset_manager_load(am, info->path);

    if (info->out_asset)
    {
        *info->out_asset = asset;
    }

    mt_free(am->engine->alloc, info);
    return 0;
}

void mt_asset_manager_queue_load(MtAssetManager *am, const char *path, MtAsset **out_asset)
{
    AssetLoadInfo *info = mt_alloc(am->engine->alloc, sizeof(AssetLoadInfo));

    info->asset_manager = am;
    info->path = path;
    info->out_asset = out_asset;

    mt_thread_pool_enqueue(&am->engine->thread_pool, asset_load, info);
}

MtAsset *mt_asset_manager_get(MtAssetManager *am, const char *path)
{
    if (am->parent)
    {
        MtAsset *asset = mt_asset_manager_get(am->parent, path);
        if (asset)
        {
            return asset;
        }
    }

    uint64_t path_hash = mt_hash_str(path);
    mt_mutex_lock(&am->mutex);
    MtAsset *asset = mt_hash_get_ptr(&am->asset_map, path_hash);
    mt_mutex_unlock(&am->mutex);
    return asset;
}

static void asset_manager_get_all_internal(
    MtAssetManager *am, MtAssetVT *type, uint32_t *count, MtAsset **assets)
{
    if (am->parent)
    {
        mt_asset_manager_get_all(am->parent, type, count, assets);
    }

    mt_mutex_lock(&am->mutex);
    for (MtAsset **asset = am->assets; asset != am->assets + mt_array_size(am->assets); ++asset)
    {
        if ((*asset)->vt == type)
        {
            if (assets)
            {
                assets[*count] = *asset;
            }
            if (count) *count += 1;
        }
    }
    mt_mutex_unlock(&am->mutex);
}

MT_ENGINE_API void
mt_asset_manager_get_all(MtAssetManager *am, MtAssetVT *type, uint32_t *count, MtAsset **assets)
{
    if (count)
    {
        *count = 0;
    }
    asset_manager_get_all_internal(am, type, count, assets);
}

void mt_asset_manager_destroy(MtAssetManager *am)
{
    mt_mutex_lock(&am->mutex);

    mt_hash_destroy(&am->asset_map);
    mt_hash_destroy(&am->asset_type_map);

    for (uint32_t i = 0; i < mt_array_size(am->assets); i++)
    {
        MtAsset *asset = am->assets[i];
        asset->vt->destroy(asset);
        mt_free(am->alloc, asset);
    }
    mt_array_free(am->alloc, am->assets);
    mt_array_free(am->alloc, am->asset_types);

    mt_mutex_unlock(&am->mutex);

    mt_mutex_destroy(&am->mutex);
}
