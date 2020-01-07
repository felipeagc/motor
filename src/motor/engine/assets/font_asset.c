#include <motor/engine/assets/font_asset.h>

#include <motor/base/util.h>
#include <motor/engine/asset_manager.h>
#include "font_asset.inl"
#include <stdio.h>

static bool asset_init(MtAssetManager *asset_manager, MtAsset *asset_, const char *path)
{
    MtFontAsset *asset = (MtFontAsset *)asset_;
    memset(asset, 0, sizeof(*asset));
    asset->asset_manager = asset_manager;

    FILE *f = fopen(path, "rb");
    if (!f)
    {
        printf("Failed to open font file: %s\n", path);
        return false;
    }

    fseek(f, 0, SEEK_END);
    size_t size = ftell(f);
    fseek(f, 0, SEEK_SET);

    asset->font_data = mt_alloc(asset_manager->alloc, size);
    fread(asset->font_data, 1, size, f);

    fclose(f);

    mt_hash_init(&asset->map, 11, asset_manager->alloc);

    asset->sampler = mt_render.create_sampler(
        asset_manager->engine->device,
        &(MtSamplerCreateInfo){.min_filter = MT_FILTER_LINEAR, .mag_filter = MT_FILTER_LINEAR});

    return true;
}

static void asset_destroy(MtAsset *asset_)
{
    MtFontAsset *asset = (MtFontAsset *)asset_;
    if (!asset)
        return;

    MtDevice *dev = asset->asset_manager->engine->device;

    for (uint32_t i = 0; i < mt_array_size(asset->atlases); i++)
    {
        FontAtlas *atlas = &asset->atlases[i];
        mt_render.destroy_image(dev, atlas->image);
        mt_free(asset->asset_manager->alloc, atlas->chardata);
    }

    mt_render.destroy_sampler(dev, asset->sampler);
    mt_free(asset->asset_manager->alloc, asset->font_data);
    mt_array_free(asset->asset_manager->alloc, asset->atlases);

    mt_hash_destroy(&asset->map);
}

static const char *g_extensions[] = {
    ".ttf",
    ".otf",
};

static MtAssetVT g_asset_vt = {
    .name            = "Font",
    .extensions      = g_extensions,
    .extension_count = MT_LENGTH(g_extensions),
    .size            = sizeof(MtFontAsset),
    .init            = asset_init,
    .destroy         = asset_destroy,
};
MtAssetVT *mt_font_asset_vt = &g_asset_vt;
