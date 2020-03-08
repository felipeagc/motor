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
#include <motor/base/api_types.h>
#include <motor/base/buffer_writer.h>
#include <motor/base/array.h>

#define KTX_UNSIGNED_BYTE 0x1401
#define KTX_RGBA 0x1908

#define KTX_COMPRESSED_RGBA_BPTC_UNORM 0x8E8C
#define KTX_COMPRESSED_SRGB_ALPHA_BPTC_UNORM 0x8E8D
#define KTX_COMPRESSED_RGB_BPTC_SIGNED_FLOAT 0x8E8E
#define KTX_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT 0x8E8F

typedef struct BC7Block
{
    uint8_t vals[16];
} BC7Block;

typedef struct ColorQuadU8
{
    uint8_t vals[4];
} ColorQuadU8;

static void buffer_writer_pad(MtBufferWriter *bw)
{
    uint64_t zero = 0;
    mt_buffer_writer_append(bw, &zero, 4 - (bw->length % 4));
}

const static uint8_t ktx_identifier[12] = {
    0xAB, 0x4B, 0x54, 0x58, 0x20, 0x31, 0x31, 0xBB, 0x0D, 0x0A, 0x1A, 0x0A};

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

int main(int argc, const char *argv[])
{
    if (argc <= 2)
    {
        printf("Usage: %s <file> <output>\n", argv[0]);
        exit(0);
    }

    const char *gltf_path = argv[1];
    const char *out_path = argv[2];

    bc7enc16_compress_block_init();

    cgltf_options gltf_options = {0};
    cgltf_data *data = NULL;
    cgltf_result result = cgltf_parse_file(&gltf_options, gltf_path, &data);

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

    MtBufferWriter bw = {0};
    mt_buffer_writer_init(&bw, NULL);

    cgltf_buffer_view **visited_buffer_views = NULL;

    for (uint32_t i = 0; i < data->images_count; i++)
    {
        cgltf_image *image = &data->images[i];
        cgltf_buffer_view *buffer_view = image->buffer_view;

        if (strcmp(image->mime_type, "image/png") == 0 ||
            strcmp(image->mime_type, "image/jpeg") == 0)
        {
            uint8_t *buffer_data = ((uint8_t *)buffer_view->buffer->data) + buffer_view->offset;
            size_t buffer_size = buffer_view->size;

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
            if (!srgb) bc7enc16_compress_block_params_init_linear_weights(&pack_params);
            pack_params.m_max_partitions_mode1 = BC7ENC16_MAX_PARTITIONS1;
            pack_params.m_uber_level = 0;

            for (uint32_t by = 0; by < blocks_y; by++)
            {
                for (uint32_t bx = 0; bx < blocks_x; bx++)
                {
                    ColorQuadU8 pixels[16];

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

                    BC7Block *block = &packed_image[bx + by * blocks_x];

                    bc7enc16_compress_block(block, pixels, &pack_params);
                }
            }

            uint64_t offset = bw.length;

            ktx_header_t header = {0};
            memcpy(&header.identifier, ktx_identifier, sizeof(ktx_identifier));
            header.endianness = 0x04030201;

            header.gl_type = KTX_UNSIGNED_BYTE;
            header.gl_type_size = 1;

            header.gl_format = KTX_RGBA;
            header.gl_base_internal_format = header.gl_format;
            if (srgb)
            {
                header.gl_internal_format = KTX_COMPRESSED_SRGB_ALPHA_BPTC_UNORM;
            }
            else
            {
                header.gl_internal_format = KTX_COMPRESSED_RGBA_BPTC_UNORM;
            }

            header.pixel_width = (uint32_t)width;
            header.pixel_height = (uint32_t)height;
            header.pixel_depth = 1;

            header.number_of_array_elements = 1;
            header.number_of_faces = 1;
            header.number_of_mipmap_levels = 1;
            header.bytes_of_key_value_data = 0;

            mt_buffer_writer_append(&bw, &header, sizeof(header));

            uint32_t image_size = mt_array_size(packed_image) * sizeof(*packed_image);
            mt_buffer_writer_append(&bw, &image_size, sizeof(image_size));

            mt_buffer_writer_append(&bw, packed_image, (uint64_t)image_size);

            buffer_writer_pad(&bw);

            assert(bw.length % 4 == 0);

            buffer_view->offset = offset;
            buffer_view->size = bw.length - offset;
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
        bool visited = false;
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
        uint64_t offset = bw.length;
        mt_buffer_writer_append(&bw, buffer_data + buffer_view->offset, buffer_view->size);
        buffer_writer_pad(&bw);
        buffer_view->offset = offset;
    }

    data->buffers_count = 1;
    data->buffers = malloc(sizeof(cgltf_buffer));
    memset(data->buffers, 0, sizeof(*data->buffers));
    data->buffers[0].size = bw.length;
    data->buffers[0].data = bw.buf;

    for (uint32_t i = 0; i < data->buffer_views_count; i++)
    {
        cgltf_buffer_view *buffer_view = &data->buffer_views[i];
        buffer_view->buffer = &data->buffers[0];
    }

    MtBufferWriter fw = {0};
    mt_buffer_writer_init(&fw, NULL);

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

    gltf_header.magic = 0x46546C67;
    gltf_header.version = 2;
    gltf_header.length = sizeof(gltf_header) + sizeof(GltfChunkHeader) + json_size +
                         sizeof(GltfChunkHeader) + bw.length;

    mt_buffer_writer_append(&fw, &gltf_header, sizeof(gltf_header));

    char *json_bytes = malloc(json_size);
    cgltf_write(NULL, json_bytes, json_size, data);
    GltfChunkHeader json_header;
    json_header.chunk_length = json_size;
    json_header.chunk_type = 0x4E4F534A;
    mt_buffer_writer_append(&fw, &json_header, sizeof(json_header));
    mt_buffer_writer_append(&fw, json_bytes, json_size);

    GltfChunkHeader bin_header;
    bin_header.chunk_length = bw.length;
    bin_header.chunk_type = 0x004E4942;
    mt_buffer_writer_append(&fw, &bin_header, sizeof(bin_header));
    mt_buffer_writer_append(&fw, bw.buf, bw.length);

    assert(fw.length == gltf_header.length);

    FILE *f = fopen(out_path, "wb");
    if (!f)
    {
        printf("Failed to open output file: %s\n", out_path);
        exit(1);
    }

    fwrite(fw.buf, 1, fw.length, f);

    fclose(f);

    return 0;
}
