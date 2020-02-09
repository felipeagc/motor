#include <motor/engine/assets/pipeline_asset.h>

#include <motor/base/util.h>
#include <motor/base/allocator.h>
#include <motor/base/array.h>
#include <motor/base/math_types.h>
#include <motor/graphics/renderer.h>
#include <motor/engine/asset_manager.h>
#include <motor/engine/engine.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "../pipeline_utils.inl"

static void asset_destroy(MtAsset *asset_)
{
    MtPipelineAsset *asset = (MtPipelineAsset *)asset_;
    if (!asset)
        return;

    mt_render.destroy_pipeline(asset->asset_manager->engine->device, asset->pipeline);
}

static bool asset_init(MtAssetManager *asset_manager, MtAsset *asset_, const char *path)
{
    MtPipelineAsset *asset = (MtPipelineAsset *)asset_;
    asset->asset_manager = asset_manager;

    // Read file
    FILE *f = fopen(path, "rb");
    if (!f)
    {
        printf("Failed to open file: %s\n", path);
        return false;
    }

    fseek(f, 0, SEEK_END);
    size_t input_size = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *input = mt_alloc(asset_manager->alloc, input_size);
    fread(input, input_size, 1, f);
    fclose(f);

    asset->pipeline = create_pipeline(asset_manager->engine, path, input, input_size);

    mt_free(asset_manager->alloc, input);

    if (!asset->pipeline)
    {
        return false;
    }

    return true;
}

static const char *g_extensions[] = {
    ".glsl",
    ".hlsl",
};

static MtAssetVT g_asset_vt = {
    .name = "Pipeline",
    .extensions = g_extensions,
    .extension_count = MT_LENGTH(g_extensions),
    .size = sizeof(MtPipelineAsset),
    .init = asset_init,
    .destroy = asset_destroy,
};
MtAssetVT *mt_pipeline_asset_vt = &g_asset_vt;
