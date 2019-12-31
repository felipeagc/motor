#include "../../../include/motor/assets/image_asset.h"

#include "../../../include/motor/allocator.h"
#include "../../../include/motor/engine.h"
#include "../../../include/motor/asset_manager.h"
#include "../../../include/motor/renderer.h"
#include "../stb_image.h"

static void image_asset_destroy(MtImageAsset *asset) {
    MtDevice *dev = asset->asset_manager->engine->device;

    mt_render.destroy_image(dev, asset->image);
    mt_render.destroy_sampler(dev, asset->sampler);

    mt_free(asset->asset_manager->alloc, asset);
}

static MtAssetVT g_image_asset_vt = {
    .destroy = (void *)image_asset_destroy,
};

MtImageAsset *
mt_image_asset_create(MtAssetManager *asset_manager, const char *path) {
    MtImageAsset *asset  = mt_alloc(asset_manager->alloc, sizeof(MtImageAsset));
    asset->asset_manager = asset_manager;

    stbi_set_flip_vertically_on_load(true);
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

    MtIAsset iasset;
    iasset.vt   = &g_image_asset_vt;
    iasset.inst = (MtAsset *)asset;

    mt_asset_manager_add(asset_manager, iasset);

    return asset;
}
