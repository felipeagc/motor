#define CGLTF_IMPLEMENTATION
#define CGLTF_WRITE_IMPLEMENTATION
#include "cgltf_write.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "bc7enc16.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <motor/base/util.h>
#include <motor/base/array.h>

// KTX defines {{{

#define KTX_ZERO 0

// Type
// Table 8.2
#define KTX_UNSIGNED_BYTE 0x1401
#define KTX_BYTE 0x1400
#define KTX_UNSIGNED_SHORT 0x1403
#define KTX_SHORT 0x1402
#define KTX_UNSIGNED_INT 0x1405
#define KTX_INT 0x1404
#define KTX_HALF_FLOAT 0x140B
#define KTX_FLOAT 0x1406
#define KTX_UNSIGNED_BYTE_3_3_2 0x8032
#define KTX_UNSIGNED_BYTE_2_3_3_REV 0x8362
#define KTX_UNSIGNED_SHORT_5_6_5 0x8363
#define KTX_UNSIGNED_SHORT_5_6_5_REV 0x8364
#define KTX_UNSIGNED_SHORT_4_4_4_4 0x8033
#define KTX_UNSIGNED_SHORT_4_4_4_4_REV 0x8365
#define KTX_UNSIGNED_SHORT_5_5_5_1 0x8034
#define KTX_UNSIGNED_SHORT_1_5_5_5_REV 0x8366
#define KTX_UNSIGNED_INT_8_8_8_8 0x8035
#define KTX_UNSIGNED_INT_8_8_8_8_REV 0x8367
#define KTX_UNSIGNED_INT_10_10_10_2 0x8036
#define KTX_UNSIGNED_INT_2_10_10_10_REV 0x8368
#define KTX_UNSIGNED_INT_24_8 0x84FA
#define KTX_UNSIGNED_INT_10F_11F_11F_REV 0x8C3B
#define KTX_UNSIGNED_INT_5_9_9_9_REV 0x8C3E
#define KTX_FLOAT_32_UNSIGNED_INT_24_8_REV 0x8DAD

// Format (done)
// Table 8.3
#define KTX_STENCIL_INDEX 0x1901
#define KTX_DEPTH_COMPONENT 0x1902
#define KTX_DEPTH_STENCIL 0x84F9
#define KTX_RED 0x1903
#define KTX_GREEN 0x1904
#define KTX_BLUE 0x1905
#define KTX_RG 0x8227
#define KTX_RGB 0x1907
#define KTX_RGBA 0x1908
#define KTX_BGR 0x80E0
#define KTX_BGRA 0x80E1
#define KTX_RED_INTEGER 0x8D94
#define KTX_GREEN_INTEGER 0x8D95
#define KTX_BLUE_INTEGER 0x8D96
#define KTX_RG_INTEGER 0x8228
#define KTX_RGB_INTEGER 0x8D98
#define KTX_RGBA_INTEGER 0x8D99
#define KTX_BGR_INTEGER 0x8D9A
#define KTX_BGRA_INTEGER 0x8D9B

// Internal format
// Table 8.14
#define KTX_COMPRESSED_RED 0x8225
#define KTX_COMPRESSED_RG 0x8226
#define KTX_COMPRESSED_RGB 0x84ED
#define KTX_COMPRESSED_RGBA 0x84EE
#define KTX_COMPRESSED_SRGB 0x8C48
#define KTX_COMPRESSED_SRGB_ALPHA 0x8C49
#define KTX_COMPRESSED_RED_RGTC1 0x8DBB
#define KTX_COMPRESSED_SIGNED_RED_RGTC1 0x8DBC
#define KTX_COMPRESSED_RG_RGTC2 0x8DBD
#define KTX_COMPRESSED_SIGNED_RG_RGTC2 0x8DBE
#define KTX_COMPRESSED_RGBA_BPTC_UNORM 0x8E8C
#define KTX_COMPRESSED_SRGB_ALPHA_BPTC_UNORM 0x8E8D
#define KTX_COMPRESSED_RGB_BPTC_SIGNED_FLOAT 0x8E8E
#define KTX_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT 0x8E8F
#define KTX_COMPRESSED_RGB8_ETC2 0x9274
#define KTX_COMPRESSED_SRGB8_ETC2 0x9275
#define KTX_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2 0x9276
#define KTX_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2 0x9277
#define KTX_COMPRESSED_RGBA8_ETC2_EAC 0x9278
#define KTX_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC 0x9279
#define KTX_COMPRESSED_R11_EAC 0x9270
#define KTX_COMPRESSED_SIGNED_R11_EAC 0x9271
#define KTX_COMPRESSED_RG11_EAC 0x9272
#define KTX_COMPRESSED_SIGNED_RG11_EAC 0x9273

// Table 8.12
#define KTX_R8 0x8229
#define KTX_R8_SNORM 0x8F94
#define KTX_R16 0x822A
#define KTX_R16_SNORM 0x8F98
#define KTX_RG8 0x822B
#define KTX_RG8_SNORM 0x8F95
#define KTX_RG16 0x822C
#define KTX_RG16_SNORM 0x8F99
#define KTX_R3_G3_B2 0x2A10
#define KTX_RGB4 0x804F
#define KTX_RGB5 0x8050
#define KTX_RGB565 0x8D62
#define KTX_RGB8 0x8051
#define KTX_RGB8_SNORM 0x8F96
#define KTX_RGB10 0x8052
#define KTX_RGB12 0x8053
#define KTX_RGB16 0x8054
#define KTX_RGB16_SNORM 0x8F9A
#define KTX_RGBA2 0x8055
#define KTX_RGBA4 0x8056
#define KTX_RGB5_A1 0x8057
#define KTX_RGBA8 0x8058
#define KTX_RGBA8_SNORM 0x8F97
#define KTX_RGB10_A2 0x8059
#define KTX_RGB10_A2UI 0x906F
#define KTX_RGBA12 0x805A
#define KTX_RGBA16 0x805B
#define KTX_RGBA16_SNORM 0x8F9B
#define KTX_SRGB8 0x8C41
#define KTX_SRGB8_ALPHA8 0x8C43
#define KTX_R16F 0x822D
#define KTX_RG16F 0x822F
#define KTX_RGB16F 0x881B
#define KTX_RGBA16F 0x881A
#define KTX_R32F 0x822E
#define KTX_RG32F 0x8230
#define KTX_RGB32F 0x8815
#define KTX_RGBA32F 0x8814
#define KTX_R11F_G11F_B10F 0x8C3A
#define KTX_RGB9_E5 0x8C3D
#define KTX_R8I 0x8231
#define KTX_R8UI 0x8232
#define KTX_R16I 0x8233
#define KTX_R16UI 0x8234
#define KTX_R32I 0x8235
#define KTX_R32UI 0x8236
#define KTX_RG8I 0x8237
#define KTX_RG8UI 0x8238
#define KTX_RG16I 0x8239
#define KTX_RG16UI 0x823A
#define KTX_RG32I 0x823B
#define KTX_RG32UI 0x823C
#define KTX_RGB8I 0x8D8F
#define KTX_RGB8UI 0x8D7D
#define KTX_RGB16I 0x8D89
#define KTX_RGB16UI 0x8D77
#define KTX_RGB32I 0x8D83
#define KTX_RGB32UI 0x8D71
#define KTX_RGBA8I 0x8D8E
#define KTX_RGBA8UI 0x8D7C
#define KTX_RGBA16I 0x8D88
#define KTX_RGBA16UI 0x8D76
#define KTX_RGBA32I 0x8D82

// Table 8.13
#define KTX_DEPTH_COMPONENT16 0x81A5
#define KTX_DEPTH_COMPONENT24 0x81A6
#define KTX_DEPTH_COMPONENT32 0x81A7
#define KTX_DEPTH_COMPONENT32F 0x8CAC
#define KTX_DEPTH24_STENCIL8 0x88F0
#define KTX_DEPTH32F_STENCIL8 0x8CAD
#define KTX_STENCIL_INDEX1 0x8D46
#define KTX_STENCIL_INDEX4 0x8D47
#define KTX_STENCIL_INDEX8 0x8D48
#define KTX_STENCIL_INDEX16 0x8D49

// Base internal format
// Table 8.11
#define KTX_DEPTH_COMPONENT 0x1902
#define KTX_DEPTH_STENCIL 0x84F9
#define KTX_RED 0x1903
#define KTX_RG 0x8227
#define KTX_RGB 0x1907
#define KTX_RGBA 0x1908
#define KTX_STENCIL_INDEX 0x1901

// }}}

typedef struct ByteWriter
{
    uint8_t *buffer;
    uint64_t capacity;
    uint64_t count;
} ByteWriter;

typedef struct BC7Block
{
    uint8_t vals[16];
} BC7Block;

typedef struct ColorQuadU8
{
    uint8_t vals[4];
} ColorQuadU8;

static void write_bytes(ByteWriter *bw, const void *bytes, uint64_t size)
{
    if (!bw->buffer)
    {
        bw->capacity = 1 << 14;
        bw->capacity = bw->capacity < size ? size : bw->capacity;
        bw->buffer   = realloc(bw->buffer, bw->capacity);
        bw->count    = 0;
    }
    if (bw->capacity - bw->count < size)
    {
        bw->capacity *= 2;
        bw->capacity += size;
        bw->buffer = realloc(bw->buffer, bw->capacity);
    }
    memcpy(bw->buffer + bw->count, bytes, size);
    bw->count += size;
}

static void byte_writer_pad(ByteWriter *bw)
{
    uint64_t zero = 0;
    write_bytes(bw, &zero, 4 - (bw->count % 4));
}

const static uint8_t ktx_identifier[12] = {
    0xAB,
    0x4B,
    0x54,
    0x58,
    0x20,
    0x31,
    0x31,
    0xBB,
    0x0D,
    0x0A,
    0x1A,
    0x0A,
};

typedef struct ktx_header_t
{
    uint8_t identifier[12];
    uint32_t endianness;
    uint32_t gl_type;
    uint32_t gl_type_size;
    uint32_t gl_format;
    uint32_t gl_internal_format;
    uint32_t gl_base_internal_format;
    uint32_t pixel_width;
    uint32_t pixel_height;
    uint32_t pixel_depth;
    uint32_t number_of_array_elements;
    uint32_t number_of_faces;
    uint32_t number_of_mipmap_levels;
    uint32_t bytes_of_key_value_data;
} ktx_header_t;

static inline void get_block(
    ColorQuadU8 *image,
    uint32_t image_width,
    uint32_t bx,
    uint32_t by,
    uint32_t width,
    uint32_t height,
    ColorQuadU8 *pixels)
{
    for (uint32_t y = 0; y < height; y++)
    {
        memcpy(
            pixels + y * width,
            &image[((bx * width) + image_width * (by * height + y))],
            width * sizeof(ColorQuadU8));
    }
}

int main(int argc, const char *argv[])
{
    if (argc <= 2)
    {
        printf("Usage: %s <file> <output>\n", argv[0]);
        exit(0);
    }

    const char *gltf_path = argv[1];
    const char *out_path  = argv[2];

    bc7enc16_compress_block_init();

    cgltf_options gltf_options = {0};
    cgltf_data *data           = NULL;
    cgltf_result result        = cgltf_parse_file(&gltf_options, gltf_path, &data);

    if (result != cgltf_result_success)
    {
        printf("Failed to parse GLTF\n");
        exit(1);
    }

    result = cgltf_load_buffers(&gltf_options, data, gltf_path);
    if (result != cgltf_result_success)
    {
        printf("Failed to load GLTF buffers\n");
        exit(1);
    }

    ByteWriter bw = {0};

    cgltf_buffer_view **visited_buffer_views = NULL;

    for (uint32_t i = 0; i < data->images_count; i++)
    {
        cgltf_image *image             = &data->images[i];
        cgltf_buffer_view *buffer_view = image->buffer_view;

        if (strcmp(image->mime_type, "image/png") == 0 ||
            strcmp(image->mime_type, "image/jpeg") == 0)
        {
            uint8_t *buffer_data = ((uint8_t *)buffer_view->buffer->data) + buffer_view->offset;
            size_t buffer_size   = buffer_view->size;

            int width, height, n_channels;
            ColorQuadU8 *image_pixels = (ColorQuadU8 *)stbi_load_from_memory(
                buffer_data, (int)buffer_size, &width, &height, &n_channels, 4);
            if (!image_pixels)
            {
                printf("Failed to load image\n");
                exit(1);
            }

            assert(width % 4 == 0);
            assert(height % 4 == 0);

            uint32_t blocks_x = (uint32_t)width >> 2;
            uint32_t blocks_y = (uint32_t)height >> 2;

            BC7Block *packed_image = NULL;
            mt_array_add(NULL, packed_image, blocks_x * blocks_y);

            bool srgb = false;

            bc7enc16_compress_block_params pack_params = {0};
            bc7enc16_compress_block_params_init(&pack_params);
            if (!srgb)
                bc7enc16_compress_block_params_init_linear_weights(&pack_params);
            pack_params.m_max_partitions_mode1 = BC7ENC16_MAX_PARTITIONS1;
            pack_params.m_uber_level           = 0;

            for (uint32_t by = 0; by < blocks_y; by++)
            {
                for (uint32_t bx = 0; bx < blocks_x; bx++)
                {
                    ColorQuadU8 pixels[16];

                    /* for (uint32_t i = 0; i < MT_LENGTH(pixels); i++) */
                    /*     printf( */
                    /*         "%02X%02X%02X%02X ", */
                    /*         pixels[i].vals[0], */
                    /*         pixels[i].vals[1], */
                    /*         pixels[i].vals[2], */
                    /*         pixels[i].vals[3]); */
                    /* printf("\n"); */

                    memcpy(
                        pixels + 0,
                        &image_pixels[bx * 4 + (uint32_t)width * (by * 4 + 0)],
                        sizeof(ColorQuadU8) * 4);
                    memcpy(
                        pixels + 4,
                        &image_pixels[bx * 4 + (uint32_t)width * (by * 4 + 1)],
                        sizeof(ColorQuadU8) * 4);
                    memcpy(
                        pixels + 8,
                        &image_pixels[bx * 4 + (uint32_t)width * (by * 4 + 2)],
                        sizeof(ColorQuadU8) * 4);
                    memcpy(
                        pixels + 12,
                        &image_pixels[bx * 4 + (uint32_t)width * (by * 4 + 3)],
                        sizeof(ColorQuadU8) * 4);

                    /* get_block(image_pixels, (uint32_t)width, bx, by, 4, 4, pixels); */

                    BC7Block *block = &packed_image[bx + by * blocks_x];

                    bc7enc16_compress_block(block, pixels, &pack_params);
                }
            }

            uint64_t offset = bw.count;

            ktx_header_t header = {0};
            memcpy(&header.identifier, ktx_identifier, sizeof(ktx_identifier));
            header.endianness = 0x04030201;

            header.gl_type      = KTX_UNSIGNED_BYTE;
            header.gl_type_size = 1;

            header.gl_format               = KTX_RGBA;
            header.gl_base_internal_format = header.gl_format;
            header.gl_internal_format      = KTX_COMPRESSED_RGBA_BPTC_UNORM;

            header.pixel_width  = (uint32_t)width;
            header.pixel_height = (uint32_t)height;
            header.pixel_depth  = 1;

            header.number_of_array_elements = 1;
            header.number_of_faces          = 1;
            header.number_of_mipmap_levels  = 1;
            header.bytes_of_key_value_data  = 0;

            write_bytes(&bw, &header, sizeof(header));

            /* const uint32_t pixel_format_bpp = 8; */
            /* uint32_t image_size = ((uint32_t)(width * height) * pixel_format_bpp) >> 3; */
            uint32_t image_size = mt_array_size(packed_image) * sizeof(*packed_image);
            write_bytes(&bw, &image_size, sizeof(image_size));

            write_bytes(&bw, packed_image, (uint64_t)image_size);

            byte_writer_pad(&bw);

            assert(bw.count % 4 == 0);

            buffer_view->offset = offset;
            buffer_view->size   = bw.count - offset;
            mt_array_push(NULL, visited_buffer_views, buffer_view);

            image->mime_type = "image/ktx";

            free(image_pixels);
        }
        else
        {
            printf("Unsupported image mime type: %s\n", image->mime_type);
            exit(1);
        }
    }

    for (uint32_t i = 0; i < data->buffer_views_count; i++)
    {
        bool visited                   = false;
        cgltf_buffer_view *buffer_view = &data->buffer_views[i];
        for (uint32_t j = 0; j < mt_array_size(visited_buffer_views); j++)
        {
            if (visited_buffer_views[j] == buffer_view)
            {
                visited = true;
                break;
            }
        }

        if (visited)
        {
            continue;
        }

        uint8_t *buffer_data = (uint8_t *)buffer_view->buffer->data;
        uint64_t offset      = bw.count;
        write_bytes(&bw, buffer_data + buffer_view->offset, buffer_view->size);
        byte_writer_pad(&bw);
        buffer_view->offset = offset;
    }

    data->buffers_count = 1;
    data->buffers       = malloc(sizeof(cgltf_buffer));
    memset(data->buffers, 0, sizeof(*data->buffers));
    data->buffers[0].size = bw.count;
    data->buffers[0].data = bw.buffer;

    for (uint32_t i = 0; i < data->buffer_views_count; i++)
    {
        cgltf_buffer_view *buffer_view = &data->buffer_views[i];
        buffer_view->buffer            = &data->buffers[0];
    }

    ByteWriter fw = {0};

    typedef struct GltfChunkHeader
    {
        uint32_t chunk_length;
        uint32_t chunk_type;
    } GltfChunkHeader;

    struct
    {
        uint32_t magic;
        uint32_t version;
        uint32_t length;
    } gltf_header;

    uint64_t json_size = cgltf_write(NULL, NULL, 0, data);

    gltf_header.magic   = 0x46546C67;
    gltf_header.version = 2;
    gltf_header.length  = sizeof(gltf_header) + sizeof(GltfChunkHeader) + json_size +
                         sizeof(GltfChunkHeader) + bw.count;

    write_bytes(&fw, &gltf_header, sizeof(gltf_header));

    char *json_bytes = malloc(json_size);
    cgltf_write(NULL, json_bytes, json_size, data);
    GltfChunkHeader json_header;
    json_header.chunk_length = json_size;
    json_header.chunk_type   = 0x4E4F534A;
    write_bytes(&fw, &json_header, sizeof(json_header));
    write_bytes(&fw, json_bytes, json_size);

    GltfChunkHeader bin_header;
    bin_header.chunk_length = bw.count;
    bin_header.chunk_type   = 0x004E4942;
    write_bytes(&fw, &bin_header, sizeof(bin_header));
    write_bytes(&fw, bw.buffer, bw.count);

    assert(fw.count == gltf_header.length);

    FILE *f = fopen(out_path, "wb");
    if (!f)
    {
        printf("Failed to open output file: %s\n", out_path);
        exit(1);
    }

    fwrite(fw.buffer, 1, fw.count, f);

    fclose(f);

    return 0;
}
