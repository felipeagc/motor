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
