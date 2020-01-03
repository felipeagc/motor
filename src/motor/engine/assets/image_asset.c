#include <motor/engine/assets/image_asset.h>

#include <motor/base/util.h>
#include <motor/base/allocator.h>
#include <motor/engine/engine.h>
#include <motor/engine/asset_manager.h>
#include <motor/graphics/renderer.h>
#include "../stb_image.h"

static bool
asset_init(MtAssetManager *asset_manager, MtAsset *asset_, const char *path) {
    MtImageAsset *asset  = (MtImageAsset *)asset_;
    asset->asset_manager = asset_manager;

    int32_t w, h, num_channels;
    uint8_t *image_data = stbi_load(path, &w, &h, &num_channels, 4);

    asset->image = mt_render.create_image(
        asset_manager->engine->device,
        &(MtImageCreateInfo){
            .width  = (uint32_t)w,
            .height = (uint32_t)h,
            .format = MT_FORMAT_RGBA8_UNORM,
        });

    mt_render.transfer_to_image(
        asset_manager->engine->device,
        &(MtImageCopyView){.image = asset->image},
        (uint32_t)(num_channels * w * h),
        image_data);

    free(image_data);

    asset->sampler = mt_render.create_sampler(
        asset_manager->engine->device,
        &(MtSamplerCreateInfo){.min_filter = MT_FILTER_NEAREST,
                               .mag_filter = MT_FILTER_NEAREST});

    return true;
}

static void asset_destroy(MtAsset *asset_) {
    MtImageAsset *asset = (MtImageAsset *)asset_;
    if (!asset) return;

    MtDevice *dev = asset->asset_manager->engine->device;

    mt_render.destroy_image(dev, asset->image);
    mt_render.destroy_sampler(dev, asset->sampler);
}

static const char *g_extensions[] = {
    ".png",
    ".jpg",
    ".jpeg",
};

static MtAssetVT g_asset_vt = {
    .name            = "Image",
    .extensions      = g_extensions,
    .extension_count = MT_LENGTH(g_extensions),
    .size            = sizeof(MtImageAsset),
    .init            = asset_init,
    .destroy         = asset_destroy,
};
MtAssetVT *mt_image_asset_vt = &g_asset_vt;
