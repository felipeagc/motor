#include <motor/base/allocator.h>
#include <motor/base/array.h>
#include <motor/graphics/renderer.h>
#include <motor/engine/engine.h>
#include "../stb_rect_pack.h"
#include "../stb_truetype.h"
#include <string.h>

typedef struct FontAtlas
{
    MtImage *image;
    stbtt_packedchar *chardata;
    uint32_t dim;
} FontAtlas;

struct MtFontAsset
{
    MtAssetManager *asset_manager;
    MtSampler *sampler;
    uint8_t *font_data;
    FontAtlas *atlases;
    MtHashMap map;
};

static FontAtlas *get_atlas(MtFontAsset *asset, uint32_t height)
{
    FontAtlas *atlas = mt_hash_get_ptr(&asset->map, (uint64_t)height);
    if (!atlas)
    {
        MtAllocator *alloc = asset->asset_manager->alloc;

        // Create atlas
        atlas = mt_array_push(alloc, asset->atlases, (FontAtlas){0});
        memset(atlas, 0, sizeof(*atlas));

        atlas->dim = 2048;

        uint8_t *pixels = mt_alloc(alloc, atlas->dim * atlas->dim);

        stbtt_pack_context spc;
        int res = stbtt_PackBegin(&spc, pixels, atlas->dim, atlas->dim, 0, 1, NULL);
        if (!res)
        {
            return NULL;
        }

        stbtt_PackSetOversampling(&spc, 2, 2);

        int first_char  = 0;
        int char_count  = 255;
        atlas->chardata = mt_alloc(alloc, sizeof(stbtt_packedchar) * (char_count - first_char));

        res = stbtt_PackFontRange(
            &spc,
            asset->font_data,
            0,
            STBTT_POINT_SIZE((int32_t)height),
            first_char,
            char_count,
            atlas->chardata);
        if (!res)
        {
            return NULL;
        }

        stbtt_PackEnd(&spc);

        atlas->image = mt_render.create_image(
            asset->asset_manager->engine->device,
            &(MtImageCreateInfo){
                .width  = atlas->dim,
                .height = atlas->dim,
                .format = MT_FORMAT_R8_UNORM,
            });

        mt_render.transfer_to_image(
            asset->asset_manager->engine->device,
            &(MtImageCopyView){.image = atlas->image},
            atlas->dim * atlas->dim,
            pixels);

        mt_free(alloc, pixels);

        mt_hash_set_ptr(&asset->map, (uint64_t)height, atlas);
    }

    return atlas;
}
