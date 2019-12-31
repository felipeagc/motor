#include "../../../include/motor/assets/pipeline_asset.h"

#include "../../../include/motor/allocator.h"
#include "../../../include/motor/asset_manager.h"

static void pipeline_asset_destroy(MtPipelineAsset *asset) {
    mt_free(asset->asset_manager->alloc, asset);
}

static MtAssetVT g_pipeline_asset_vt = {
    .destroy = (void *)pipeline_asset_destroy,
};

MtIAsset *
mt_pipeline_asset_create(MtAssetManager *asset_manager, const char *path) {
    MtPipelineAsset *asset =
        mt_alloc(asset_manager->alloc, sizeof(MtPipelineAsset));
    asset->asset_manager = asset_manager;

    MtIAsset iasset;
    iasset.vt   = &g_pipeline_asset_vt;
    iasset.inst = (MtAsset *)asset;

    return mt_asset_manager_add(asset_manager, iasset);
}
