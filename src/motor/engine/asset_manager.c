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

void mt_asset_manager_init(MtAssetManager *am, MtEngine *engine)
{
    memset(am, 0, sizeof(*am));
    am->engine = engine;
    am->alloc = engine->alloc;

    mt_mutex_init(&am->mutex);

    mt_hash_init(&am->asset_map, 51, am->alloc);

    mt_array_push(am->alloc, am->asset_types, mt_image_asset_vt);
    mt_array_push(am->alloc, am->asset_types, mt_pipeline_asset_vt);
    mt_array_push(am->alloc, am->asset_types, mt_font_asset_vt);
    mt_array_push(am->alloc, am->asset_types, mt_gltf_asset_vt);
}

MtAsset *mt_asset_manager_load(MtAssetManager *am, const char *path)
{
    mt_render.set_thread_id(mt_thread_pool_get_task_id());

    for (uint32_t i = 0; i < mt_array_size(am->asset_types); i++)
    {
        MtAssetVT *vt = am->asset_types[i];

        for (uint32_t j = 0; j < vt->extension_count; j++)
        {
            const char *ext = vt->extensions[j];
            const char *path_ext = mt_path_ext(path);

            if (strcmp(ext, path_ext) == 0)
            {
                mt_log_debug("Loading asset: %s", path);

                MtAsset *temp_asset_ptr = mt_alloc(am->alloc, vt->size);

                bool initialized = vt->init(am, temp_asset_ptr, path);
                if (!initialized)
                {
                    mt_log_error("Failed to load asset: %s", path);
                    mt_free(am->alloc, temp_asset_ptr);
                    temp_asset_ptr = NULL;
                }

                uint64_t path_hash = mt_hash_str(path);

                mt_mutex_lock(&am->mutex);
                MtIAsset *existing = mt_hash_get_ptr(&am->asset_map, path_hash);
                mt_mutex_unlock(&am->mutex);

                if (existing)
                {
                    if (temp_asset_ptr)
                    {
                        mt_mutex_lock(&am->mutex);

                        existing->vt->destroy(existing->inst);
                        memcpy(existing->inst, temp_asset_ptr, vt->size);
                        mt_free(am->alloc, temp_asset_ptr);

                        mt_mutex_unlock(&am->mutex);
                    }

                    return existing->inst;
                }
                else
                {
                    if (temp_asset_ptr)
                    {
                        mt_mutex_lock(&am->mutex);

                        MtIAsset iasset = {
                            .vt = vt,
                            .inst = temp_asset_ptr,
                        };
                        mt_array_push(am->alloc, am->assets, iasset);
                        MtIAsset *iasset_ptr = mt_array_last(am->assets);
                        mt_hash_set_ptr(&am->asset_map, path_hash, iasset_ptr);

                        mt_mutex_unlock(&am->mutex);
                        return iasset_ptr->inst;
                    }

                    mt_log_error("Failed to load asset: %s", path);
                    return NULL;
                }

                mt_log_error("Failed to load asset: %s", path);
                return NULL;
            }
        }
    }

    mt_log_error("No asset loader found for file: %s", path);

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
    uint64_t path_hash = mt_hash_str(path);
    mt_mutex_lock(&am->mutex);
    MtIAsset *iasset = mt_hash_get_ptr(&am->asset_map, path_hash);
    mt_mutex_unlock(&am->mutex);
    return iasset->inst;
}

void mt_asset_manager_destroy(MtAssetManager *am)
{
    mt_mutex_lock(&am->mutex);

    mt_hash_destroy(&am->asset_map);

    for (uint32_t i = 0; i < mt_array_size(am->assets); i++)
    {
        MtIAsset *asset = &am->assets[i];
        asset->vt->destroy(asset->inst);
        mt_free(am->alloc, asset->inst);
    }
    mt_array_free(am->alloc, am->assets);
    mt_array_free(am->alloc, am->asset_types);

    mt_mutex_unlock(&am->mutex);

    mt_mutex_destroy(&am->mutex);
}
