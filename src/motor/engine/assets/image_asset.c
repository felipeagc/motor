#include <motor/engine/assets/image_asset.h>

#include <string.h>
#include <assert.h>
#include <motor/base/util.h>
#include <motor/base/allocator.h>
#include <motor/graphics/renderer.h>
#include <motor/engine/engine.h>
#include <motor/engine/asset_manager.h>
#include "../stb_image.h"
#include "../tinyktx.h"

static bool asset_init(MtAssetManager *asset_manager, MtAsset *asset_, const char *path)
{
    MtImageAsset *asset  = (MtImageAsset *)asset_;
    asset->asset_manager = asset_manager;

    const char *ext = mt_path_ext(path);
    if (strcmp(ext, ".png") == 0 || strcmp(ext, ".jpg") == 0 || strcmp(ext, ".jpeg") == 0)
    {
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

        return true;
    }

    if (strcmp(ext, ".ktx") == 0)
    {
        ktx_data_t data     = {0};
        uint8_t *raw_data   = NULL;
        ktx_result_t result = ktx_read_from_file(path, &raw_data, &data);
        if (result != KTX_SUCCESS)
        {
            return false;
        }

        uint32_t width  = data.pixel_width;
        uint32_t height = data.pixel_height;

        uint32_t block_size = 0;
        MtFormat format;
        switch (data.internal_format)
        {
            case KTX_RGBA8:
                format     = MT_FORMAT_RGBA8_UNORM;
                block_size = sizeof(uint32_t);
                break;
            case KTX_RGBA16F:
                format     = MT_FORMAT_RGBA16_SFLOAT;
                block_size = 2 * sizeof(uint32_t);
                break;
            case KTX_COMPRESSED_RGBA_BPTC_UNORM:
                format     = MT_FORMAT_BC7_UNORM_BLOCK;
                block_size = 16;
                width >>= 2;
                height >>= 2;
                break;
            default: assert(!"Unsupported image format");
        }

        asset->image = mt_render.create_image(
            asset_manager->engine->device,
            &(MtImageCreateInfo){
                .width       = data.pixel_width,
                .height      = data.pixel_height,
                .depth       = data.pixel_depth,
                .mip_count   = data.mipmap_level_count,
                .layer_count = data.face_count,
                .format      = format,
            });

        for (uint32_t li = 0; li < data.mipmap_level_count; li++)
        {
            for (uint32_t fi = 0; fi < data.face_count; fi++)
            {
                for (uint32_t si = 0; si < data.pixel_depth; si++)
                {
                    uint32_t mip_width  = width >> li;
                    uint32_t mip_height = height >> li;

                    ktx_slice_t *slice =
                        &data.mip_levels[li].array_elements[0].faces[fi].slices[si];

                    mt_render.transfer_to_image(
                        asset_manager->engine->device,
                        &(MtImageCopyView){.image       = asset->image,
                                           .mip_level   = li,
                                           .array_layer = fi,
                                           .offset      = {.z = si}},
                        mip_width * mip_height * block_size,
                        slice->data);
                }
            }
        }

        free(raw_data);
        ktx_data_destroy(&data);

        return true;
    }

    return false;
}

static void asset_destroy(MtAsset *asset_)
{
    MtImageAsset *asset = (MtImageAsset *)asset_;
    if (!asset)
        return;

    MtDevice *dev = asset->asset_manager->engine->device;

    mt_render.destroy_image(dev, asset->image);
}

static const char *g_extensions[] = {
    ".png",
    ".jpg",
    ".jpeg",
    ".ktx",
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
