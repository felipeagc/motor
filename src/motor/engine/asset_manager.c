#include <motor/engine/asset_manager.h>

#include <stdio.h>
#include <string.h>
#include <motor/base/util.h>
#include <motor/base/allocator.h>
#include <motor/base/array.h>
#include <motor/engine/engine.h>

#include <motor/engine/assets/image_asset.h>
#include <motor/engine/assets/pipeline_asset.h>
#include <motor/engine/assets/font_asset.h>
#include <motor/engine/assets/gltf_asset.h>

void mt_asset_manager_init(MtAssetManager *am, MtEngine *engine)
{
    memset(am, 0, sizeof(*am));
    am->engine = engine;
    am->alloc  = engine->alloc;

    mt_hash_init(&am->asset_map, 51, am->alloc);

    mt_array_push(am->alloc, am->asset_types, mt_image_asset_vt);
    mt_array_push(am->alloc, am->asset_types, mt_pipeline_asset_vt);
    mt_array_push(am->alloc, am->asset_types, mt_font_asset_vt);
    mt_array_push(am->alloc, am->asset_types, mt_gltf_asset_vt);
}

MtAsset *mt_asset_manager_load(MtAssetManager *am, const char *path)
{
    for (uint32_t i = 0; i < mt_array_size(am->asset_types); i++)
    {
        MtAssetVT *vt = am->asset_types[i];

        for (uint32_t j = 0; j < vt->extension_count; j++)
        {
            const char *ext      = vt->extensions[j];
            const char *path_ext = mt_path_ext(path, strlen(path));

            if (strcmp(ext, path_ext) == 0)
            {
                uint64_t path_hash = mt_hash_str(path);
                MtIAsset *existing = mt_hash_get_ptr(&am->asset_map, path_hash);

                printf("Loading asset: %s\n", path);

                MtAsset *temp_asset_ptr = mt_alloc(am->alloc, vt->size);

                bool initialized = vt->init(am, temp_asset_ptr, path);
                if (!initialized)
                {
                    printf("Failed to load asset: %s\n", path);
                    mt_free(am->alloc, temp_asset_ptr);
                    temp_asset_ptr = NULL;
                }

                if (existing)
                {
                    if (temp_asset_ptr)
                    {
                        existing->vt->destroy(existing->inst);
                        memcpy(existing->inst, temp_asset_ptr, vt->size);
                        mt_free(am->alloc, temp_asset_ptr);
                    }

                    return existing->inst;
                }
                else
                {
                    if (temp_asset_ptr)
                    {
                        MtIAsset iasset = {
                            .vt   = vt,
                            .inst = temp_asset_ptr,
                        };
                        MtIAsset *iasset_ptr = mt_array_push(am->alloc, am->assets, iasset);
                        mt_hash_set_ptr(&am->asset_map, path_hash, iasset_ptr);
                        return iasset_ptr->inst;
                    }

                    return NULL;
                }

                return NULL;
            }
        }
    }

    printf("No asset loader found for file: %s\n", path);

    return NULL;
}

void mt_asset_manager_destroy(MtAssetManager *am)
{
    mt_hash_destroy(&am->asset_map);

    for (uint32_t i = 0; i < mt_array_size(am->assets); i++)
    {
        MtIAsset *asset = &am->assets[i];
        asset->vt->destroy(asset->inst);
        mt_free(am->alloc, asset->inst);
    }
    mt_array_free(am->alloc, am->assets);
}
