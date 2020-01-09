#include <motor/engine/assets/gltf_asset.h>

#include "../stb_image.h"
#define CGLTF_IMPLEMENTATION
#include "cgltf.h"
#include <motor/base/util.h>
#include <motor/base/array.h>
#include <motor/base/allocator.h>
#include <motor/base/math.h>
#include <motor/graphics/renderer.h>
#include <motor/engine/asset_manager.h>
#include <motor/engine/engine.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

typedef struct GltfVertex
{
    Vec3 pos;
    Vec3 normal;
    Vec2 uv0;
    Vec4 tangent;
} GltfVertex;

typedef struct MaterialUniform
{
    Vec4 base_color_factor;
    float metallic;
    float roughness;
    Vec4 emissive_factor;
} MaterialUniform;

typedef struct GltfMaterial
{
    MaterialUniform uniform;

    MtImage *albedo_image;
    MtSampler *albedo_sampler;

    MtImage *normal_image;
    MtSampler *normal_sampler;

    MtImage *metallic_roughness_image;
    MtSampler *metallic_roughness_sampler;

    MtImage *occlusion_image;
    MtSampler *occlusion_sampler;

    MtImage *emissive_image;
    MtSampler *emissive_sampler;
} GltfMaterial;

typedef struct GltfPrimitive
{
    uint32_t first_index;
    uint32_t index_count;
    uint32_t vertex_count;
    GltfMaterial *material;
    bool has_indices;
} GltfPrimitive;

typedef struct GltfMesh
{
    /*array*/ GltfPrimitive *primitives;
    Mat4 matrix;
} GltfMesh;

typedef struct GltfNode
{
    struct GltfNode *parent;
    /*array*/ struct GltfNode **children;

    Mat4 matrix;
    GltfMesh *mesh;

    Vec3 translation;
    Vec3 scale;
    Quat rotation;
} GltfNode;

struct MtGltfAsset
{
    MtAssetManager *asset_manager;

    /*array*/ GltfNode **nodes;
    /*array*/ GltfNode **linear_nodes;

    /*array*/ MtImage **images;
    /*array*/ MtSampler **samplers;
    /*array*/ GltfMaterial *materials;

    MtBuffer *vertex_buffer;

    uint32_t index_count;
    MtBuffer *index_buffer;
};

static void load_node(
    MtGltfAsset *asset,
    GltfNode *parent,
    cgltf_node *node,
    cgltf_data *model,
    uint32_t **index_buffer,
    GltfVertex **vertex_buffer);

static Mat4 node_local_matrix(GltfNode *node)
{
    Mat4 result = mat4_identity();
    result      = mat4_translate(result, node->translation);
    result      = mat4_mul(result, quat_to_mat4(node->rotation));
    result      = mat4_scale(result, node->scale);
    result      = mat4_mul(result, node->matrix);
    return result;
}

static Mat4 node_get_matrix(GltfNode *node)
{
    Mat4 m      = node_local_matrix(node);
    GltfNode *p = node->parent;
    while (p)
    {
        m = mat4_mul(node_local_matrix(p), m);
        p = p->parent;
    }
    return m;
}

static void node_update(GltfNode *node)
{
    if (node->mesh)
    {
        node->mesh->matrix = node_get_matrix(node);
    }

    for (uint32_t i = 0; i < mt_array_size(node->children); i++)
    {
        node_update(node->children[i]);
    }
}

static bool asset_init(MtAssetManager *asset_manager, MtAsset *asset_, const char *path)
{
    MtGltfAsset *asset = (MtGltfAsset *)asset_;
    memset(asset, 0, sizeof(*asset));
    asset->asset_manager = asset_manager;

    MtAllocator *alloc = asset_manager->alloc;

    FILE *f = fopen(path, "rb");
    if (!f)
    {
        return false;
    }

    fseek(f, 0, SEEK_END);
    size_t gltf_size = ftell(f);
    fseek(f, 0, SEEK_SET);

    uint8_t *gltf_data = mt_alloc(alloc, gltf_size);
    fread(gltf_data, 1, gltf_size, f);

    fclose(f);

    cgltf_options gltf_options = {0};
    cgltf_data *data           = NULL;
    cgltf_result result        = cgltf_parse(&gltf_options, gltf_data, gltf_size, &data);
    if (result != cgltf_result_success)
    {
        mt_free(alloc, gltf_data);
        return false;
    }

    result = cgltf_load_buffers(&gltf_options, data, path);
    if (result != cgltf_result_success)
    {
        cgltf_free(data);
        mt_free(alloc, gltf_data);
        return false;
    }

    // TODO: support .gltf
    assert(data->file_type == cgltf_file_type_glb);

    // Load samplers
    mt_array_pushn(alloc, asset->samplers, data->samplers_count);
    for (uint32_t i = 0; i < data->samplers_count; i++)
    {
        cgltf_sampler *sampler = &data->samplers[i];
        MtSamplerCreateInfo ci = {0};
        ci.anisotropy          = true;
        switch (sampler->mag_filter)
        {
            case 0x2601:
            {
                ci.mag_filter = MT_FILTER_LINEAR;
            }
            break;
            case 0x2600:
            {
                ci.mag_filter = MT_FILTER_NEAREST;
            }
            break;
            default: break;
        }

        switch (sampler->min_filter)
        {
            case 0x2601:
            {
                ci.min_filter = MT_FILTER_LINEAR;
            }
            break;
            case 0x2600:
            {
                ci.min_filter = MT_FILTER_NEAREST;
            }
            break;
            default: break;
        }

        asset->samplers[i] = mt_render.create_sampler(asset_manager->engine->device, &ci);
    }

    // Load images
    mt_array_pushn(alloc, asset->images, data->images_count);
    for (uint32_t i = 0; i < data->images_count; i++)
    {
        cgltf_image *image   = &data->images[i];
        uint8_t *buffer_data = image->buffer_view->buffer->data + image->buffer_view->offset;
        size_t buffer_size   = image->buffer_view->size;

        int width, height, n_channels;
        uint8_t *image_data =
            stbi_load_from_memory(buffer_data, (int)buffer_size, &width, &height, &n_channels, 4);
        assert(image_data);

        asset->images[i] = mt_render.create_image(
            asset_manager->engine->device,
            &(MtImageCreateInfo){
                .width  = (uint32_t)width,
                .height = (uint32_t)height,
                .format = MT_FORMAT_RGBA8_UNORM,
            });

        mt_render.transfer_to_image(
            asset_manager->engine->device,
            &(MtImageCopyView){.image = asset->images[i]},
            (uint32_t)(4 * width * height),
            image_data);

        stbi_image_free(image_data);
    }

    // Load materials
    mt_array_pushn(alloc, asset->materials, data->materials_count);
    for (uint32_t i = 0; i < data->materials_count; i++)
    {
        cgltf_material *material = &data->materials[i];
        assert(material->has_pbr_metallic_roughness);

        GltfMaterial *mat = &asset->materials[i];
        memset(mat, 0, sizeof(*mat));

        mat->uniform.base_color_factor = V4(1.0f, 1.0f, 1.0f, 1.0f);
        mat->uniform.metallic          = 1.0f;
        mat->uniform.roughness         = 1.0f;
        mat->uniform.emissive_factor   = V4(1.0f, 1.0f, 1.0f, 1.0f);

        if (material->pbr_metallic_roughness.base_color_texture.texture != NULL)
        {
            uint32_t image_index =
                material->pbr_metallic_roughness.base_color_texture.texture->image - data->images;
            mat->albedo_image = asset->images[image_index];

            uint32_t sampler_index =
                material->pbr_metallic_roughness.base_color_texture.texture->sampler -
                data->samplers;
            mat->albedo_sampler = asset->samplers[sampler_index];
        }
        else
        {
            mat->albedo_image   = asset->asset_manager->engine->white_image;
            mat->albedo_sampler = asset->asset_manager->engine->default_sampler;
        }

        if (material->normal_texture.texture != NULL)
        {
            uint32_t image_index = material->normal_texture.texture->image - data->images;
            mat->normal_image    = asset->images[image_index];

            uint32_t sampler_index = material->normal_texture.texture->sampler - data->samplers;
            mat->normal_sampler    = asset->samplers[sampler_index];
        }
        else
        {
            mat->normal_image   = asset->asset_manager->engine->white_image;
            mat->normal_sampler = asset->asset_manager->engine->default_sampler;
        }

        if (material->pbr_metallic_roughness.metallic_roughness_texture.texture != NULL)
        {
            uint32_t image_index =
                material->pbr_metallic_roughness.metallic_roughness_texture.texture->image -
                data->images;
            mat->metallic_roughness_image = asset->images[image_index];

            uint32_t sampler_index =
                material->pbr_metallic_roughness.metallic_roughness_texture.texture->sampler -
                data->samplers;
            mat->metallic_roughness_sampler = asset->samplers[sampler_index];
        }
        else
        {
            mat->metallic_roughness_image   = asset->asset_manager->engine->white_image;
            mat->metallic_roughness_sampler = asset->asset_manager->engine->default_sampler;
        }

        if (material->occlusion_texture.texture != NULL)
        {
            uint32_t image_index = material->occlusion_texture.texture->image - data->images;
            mat->occlusion_image = asset->images[image_index];

            uint32_t sampler_index = material->occlusion_texture.texture->sampler - data->samplers;
            mat->occlusion_sampler = asset->samplers[sampler_index];
        }
        else
        {
            mat->occlusion_image   = asset->asset_manager->engine->white_image;
            mat->occlusion_sampler = asset->asset_manager->engine->default_sampler;
        }

        if (material->emissive_texture.texture != NULL)
        {
            uint32_t image_index = material->emissive_texture.texture->image - data->images;
            mat->emissive_image  = asset->images[image_index];

            uint32_t sampler_index = material->emissive_texture.texture->sampler - data->samplers;
            mat->emissive_sampler  = asset->samplers[sampler_index];
        }
        else
        {
            mat->emissive_image   = asset->asset_manager->engine->black_image;
            mat->emissive_sampler = asset->asset_manager->engine->default_sampler;
        }
    }

    // Load nodes
    GltfVertex *vertices = NULL;
    uint32_t *indices    = NULL;

    cgltf_scene *scene = data->scene;
    for (uint32_t i = 0; i < scene->nodes_count; i++)
    {
        cgltf_node *node = scene->nodes[i];
        load_node(asset, NULL, node, data, &indices, &vertices);
    }

    for (uint32_t i = 0; i < mt_array_size(asset->linear_nodes); i++)
    {
        GltfNode *node = asset->linear_nodes[i];
        if (node->mesh)
        {
            node_update(node);
        }
    }

    size_t vertex_buffer_size = mt_array_size(vertices) * sizeof(GltfVertex);
    size_t index_buffer_size  = mt_array_size(indices) * sizeof(uint32_t);
    asset->index_count        = mt_array_size(indices);

    assert(vertex_buffer_size > 0);

    MtDevice *dev = asset->asset_manager->engine->device;

    asset->vertex_buffer = mt_render.create_buffer(
        dev,
        &(MtBufferCreateInfo){
            .usage  = MT_BUFFER_USAGE_VERTEX,
            .memory = MT_BUFFER_MEMORY_DEVICE,
            .size   = vertex_buffer_size,
        });

    asset->index_buffer = mt_render.create_buffer(
        dev,
        &(MtBufferCreateInfo){
            .usage  = MT_BUFFER_USAGE_INDEX,
            .memory = MT_BUFFER_MEMORY_DEVICE,
            .size   = index_buffer_size,
        });

    mt_render.transfer_to_buffer(dev, asset->vertex_buffer, 0, vertex_buffer_size, vertices);
    mt_render.transfer_to_buffer(dev, asset->index_buffer, 0, index_buffer_size, indices);

    mt_array_free(alloc, vertices);
    mt_array_free(alloc, indices);

    cgltf_free(data);
    mt_free(alloc, gltf_data);

    return true;
}

static void asset_destroy(MtAsset *asset_)
{
    MtGltfAsset *asset = (MtGltfAsset *)asset_;
    if (!asset)
        return;

    MtDevice *dev      = asset->asset_manager->engine->device;
    MtAllocator *alloc = asset->asset_manager->alloc;

    for (uint32_t i = 0; i < mt_array_size(asset->linear_nodes); i++)
    {
        GltfMesh *mesh = asset->linear_nodes[i]->mesh;
        if (mesh)
        {
            mt_array_free(alloc, mesh->primitives);
            mt_free(alloc, mesh);
        }
        mt_free(alloc, asset->linear_nodes[i]);
    }
    mt_array_free(alloc, asset->linear_nodes);
    mt_array_free(alloc, asset->nodes);

    mt_array_free(alloc, asset->materials);

    for (uint32_t i = 0; i < mt_array_size(asset->images); i++)
    {
        mt_render.destroy_image(dev, asset->images[i]);
    }
    mt_array_free(alloc, asset->images);

    for (uint32_t i = 0; i < mt_array_size(asset->samplers); i++)
    {
        mt_render.destroy_sampler(dev, asset->samplers[i]);
    }
    mt_array_free(alloc, asset->samplers);

    mt_render.destroy_buffer(dev, asset->vertex_buffer);
    mt_render.destroy_buffer(dev, asset->index_buffer);
}

static void load_node(
    MtGltfAsset *asset,
    GltfNode *parent,
    cgltf_node *node,
    cgltf_data *model,
    uint32_t **index_buffer,
    GltfVertex **vertex_buffer)
{
    MtAllocator *alloc = asset->asset_manager->alloc;

    GltfNode *new_node = mt_alloc(alloc, sizeof(GltfNode));
    memset(new_node, 0, sizeof(*new_node));

    new_node->parent      = parent;
    new_node->matrix      = mat4_identity();
    new_node->translation = V3(0.0f, 0.0f, 0.0f);
    new_node->rotation    = (Quat){0.0f, 0.0f, 0.0f, 1.0f};
    new_node->scale       = V3(1.0f, 1.0f, 1.0f);

    if (node->has_translation)
    {
        memcpy(&new_node->translation, node->translation, sizeof(Vec3));
    }

    if (node->has_rotation)
    {
        memcpy(&new_node->rotation, node->rotation, sizeof(Quat));
    }

    if (node->has_scale)
    {
        memcpy(&new_node->scale, node->scale, sizeof(Vec3));
    }

    if (node->has_matrix)
    {
        memcpy(&new_node->matrix, node->matrix, sizeof(Mat4));
    }

    if (node->children_count > 0)
    {
        for (uint32_t i = 0; i < node->children_count; i++)
        {
            load_node(asset, new_node, node->children[i], model, index_buffer, vertex_buffer);
        }
    }

    if (node->mesh != NULL)
    {
        cgltf_mesh *mesh   = node->mesh;
        GltfMesh *new_mesh = mt_alloc(alloc, sizeof(GltfMesh));
        memset(new_mesh, 0, sizeof(*new_mesh));

        new_mesh->matrix = new_node->matrix;

        for (uint32_t i = 0; i < mesh->primitives_count; i++)
        {
            cgltf_primitive *primitive = &mesh->primitives[i];

            uint32_t index_start  = (uint32_t)mt_array_size(*index_buffer);
            uint32_t vertex_start = (uint32_t)mt_array_size(*vertex_buffer);

            uint32_t index_count  = 0;
            uint32_t vertex_count = 0;

            bool has_indices = primitive->indices != NULL;

            // Vertices
            {
                cgltf_accessor *pos_accessor = NULL;
                cgltf_buffer_view *pos_view  = NULL;
                uint32_t pos_byte_stride     = 0;
                uint8_t *buffer_pos          = NULL;

                cgltf_accessor *normal_accessor = NULL;
                cgltf_buffer_view *normal_view  = NULL;
                uint32_t normal_byte_stride     = 0;
                uint8_t *buffer_normals         = NULL;

                cgltf_accessor *tangent_accessor = NULL;
                cgltf_buffer_view *tangent_view  = NULL;
                uint32_t tangent_byte_stride     = 0;
                uint8_t *buffer_tangents         = NULL;

                cgltf_accessor *uv0_accessor = NULL;
                cgltf_buffer_view *uv0_view  = NULL;
                uint32_t uv0_byte_stride     = 0;
                uint8_t *buffer_uv0          = NULL;

                for (uint32_t j = 0; j < primitive->attributes_count; j++)
                {
                    if (primitive->attributes[j].type == cgltf_attribute_type_position)
                    {
                        pos_accessor    = primitive->attributes[j].data;
                        pos_view        = pos_accessor->buffer_view;
                        pos_byte_stride = pos_accessor->stride;
                        buffer_pos =
                            &pos_view->buffer->data[pos_accessor->offset + pos_view->offset];

                        vertex_count = (uint32_t)pos_accessor->count;
                    }

                    if (primitive->attributes[j].type == cgltf_attribute_type_normal)
                    {
                        normal_accessor    = primitive->attributes[j].data;
                        normal_view        = normal_accessor->buffer_view;
                        normal_byte_stride = normal_accessor->stride;
                        buffer_normals     = &normal_view->buffer
                                              ->data[normal_accessor->offset + normal_view->offset];
                    }

                    if (primitive->attributes[j].type == cgltf_attribute_type_tangent)
                    {
                        tangent_accessor    = primitive->attributes[j].data;
                        tangent_view        = tangent_accessor->buffer_view;
                        tangent_byte_stride = tangent_accessor->stride;
                        buffer_tangents =
                            &tangent_view->buffer
                                 ->data[tangent_accessor->offset + tangent_view->offset];
                    }

                    if (primitive->attributes[j].type == cgltf_attribute_type_texcoord)
                    {
                        uv0_accessor    = primitive->attributes[j].data;
                        uv0_view        = uv0_accessor->buffer_view;
                        uv0_byte_stride = uv0_accessor->stride;
                        buffer_uv0 =
                            &uv0_view->buffer->data[uv0_accessor->offset + uv0_view->offset];
                    }
                }

                uint32_t first_vertex = mt_array_size(*vertex_buffer);
                mt_array_pushn(alloc, *vertex_buffer, pos_accessor->count);
                for (size_t v = 0; v < pos_accessor->count; v++)
                {
                    GltfVertex *vertex = &(*vertex_buffer)[first_vertex];

                    // Position
                    memcpy(&vertex->pos, &buffer_pos[v * pos_byte_stride], sizeof(vertex->pos));

                    // Normal
                    memcpy(
                        &vertex->normal,
                        &buffer_normals[v * normal_byte_stride],
                        sizeof(vertex->normal));

                    // Tangent
                    if (buffer_tangents)
                    {
                        memcpy(
                            &vertex->tangent,
                            &buffer_tangents[v * tangent_byte_stride],
                            sizeof(vertex->tangent));
                    }

                    // UV0
                    if (buffer_uv0)
                    {
                        memcpy(&vertex->uv0, &buffer_uv0[v * uv0_byte_stride], sizeof(vertex->uv0));
                    }

                    ++first_vertex;
                }

                if (!buffer_tangents && buffer_uv0)
                {
                    // Generate tangents
                    assert(pos_accessor->count % 3 == 0);

                    Vec3 *positions = (Vec3 *)buffer_pos;
                    Vec2 *uvs       = (Vec2 *)buffer_uv0;

                    for (uint32_t i = 0; i < pos_accessor->count / 3; i++)
                    {
                        // Loop per triangle
                        Vec3 v0 = positions[i + 0];
                        Vec3 v1 = positions[i + 1];
                        Vec3 v2 = positions[i + 2];

                        Vec2 uv0 = uvs[i + 0];
                        Vec2 uv1 = uvs[i + 1];
                        Vec2 uv2 = uvs[i + 2];

                        Vec3 delta_pos1 = v3_sub(v1, v0);
                        Vec3 delta_pos2 = v3_sub(v2, v0);

                        Vec2 delta_uv1 = v2_sub(uv1, uv0);
                        Vec2 delta_uv2 = v2_sub(uv2, uv0);

                        float r = 1.0f / (delta_uv1.x * delta_uv2.y - delta_uv1.y * delta_uv2.x);

                        Vec4 tangent;
                        tangent.w   = 1.0f;
                        tangent.xyz = v3_muls(
                            v3_sub(
                                v3_muls(delta_pos1, delta_uv2.y), v3_muls(delta_pos2, delta_uv1.y)),
                            r);

                        (*vertex_buffer)[i + 0].tangent = tangent;
                        (*vertex_buffer)[i + 1].tangent = tangent;
                        (*vertex_buffer)[i + 2].tangent = tangent;
                    }
                }
            }

            // Indices
            if (has_indices)
            {
                cgltf_accessor *accessor       = primitive->indices;
                cgltf_buffer_view *buffer_view = accessor->buffer_view;
                cgltf_buffer *buffer           = buffer_view->buffer;

                index_count          = (uint32_t)accessor->count;
                const void *data_ptr = &buffer->data[accessor->offset + buffer_view->offset];

                uint32_t first_index = mt_array_size(*index_buffer);
                mt_array_pushn(alloc, *index_buffer, accessor->count);

                switch (accessor->component_type)
                {
                    case cgltf_component_type_r_32u:
                    {
                        const uint32_t *buf = data_ptr;
                        for (size_t index = 0; index < accessor->count; index++)
                        {
                            (*index_buffer)[first_index] = buf[index] + vertex_start;
                            ++first_index;
                        }
                    }
                    break;
                    case cgltf_component_type_r_16u:
                    {
                        const uint16_t *buf = data_ptr;
                        for (size_t index = 0; index < accessor->count; index++)
                        {
                            (*index_buffer)[first_index] = ((uint32_t)buf[index]) + vertex_start;
                            ++first_index;
                        }
                    }
                    break;
                    case cgltf_component_type_r_8u:
                    {
                        const uint8_t *buf = data_ptr;
                        for (size_t index = 0; index < accessor->count; index++)
                        {
                            (*index_buffer)[first_index] = ((uint32_t)buf[index]) + vertex_start;
                            ++first_index;
                        }
                    }
                    break;
                    default:
                    {
                        assert(!"Invalid component type");
                    }
                    break;
                }
            }

            GltfPrimitive new_primitive = {0};
            new_primitive.first_index   = index_start;
            new_primitive.index_count   = index_count;
            new_primitive.vertex_count  = vertex_count;
            if (primitive->material)
            {
                new_primitive.material = &asset->materials[primitive->material - model->materials];
            }
            new_primitive.has_indices = (index_count > 0);

            mt_array_push(alloc, new_mesh->primitives, new_primitive);
        }

        new_node->mesh = new_mesh;
    }

    if (parent)
    {
        mt_array_push(alloc, parent->children, new_node);
    }
    else
    {
        mt_array_push(alloc, asset->nodes, new_node);
    }

    mt_array_push(alloc, asset->linear_nodes, new_node);
}

static void node_draw(
    GltfNode *node, MtCmdBuffer *cb, Mat4 *transform, uint32_t model_set, uint32_t material_set)
{
    if (node->mesh)
    {
        for (uint32_t i = 0; i < mt_array_size(node->mesh->primitives); i++)
        {
            GltfPrimitive *primitive = &node->mesh->primitives[i];

            struct
            {
                Mat4 local_model;
                Mat4 model;
            } uniform;
            uniform.local_model = node->mesh->matrix;
            uniform.model       = *transform;

            mt_render.cmd_bind_uniform(cb, &uniform, sizeof(uniform), model_set, 0);

            mt_render.cmd_bind_uniform(
                cb,
                &primitive->material->uniform,
                sizeof(primitive->material->uniform),
                material_set,
                0);
            mt_render.cmd_bind_image(
                cb,
                primitive->material->albedo_image,
                primitive->material->albedo_sampler,
                material_set,
                1);
            mt_render.cmd_bind_image(
                cb,
                primitive->material->normal_image,
                primitive->material->normal_sampler,
                material_set,
                2);
            mt_render.cmd_bind_image(
                cb,
                primitive->material->metallic_roughness_image,
                primitive->material->metallic_roughness_sampler,
                material_set,
                3);
            mt_render.cmd_bind_image(
                cb,
                primitive->material->occlusion_image,
                primitive->material->occlusion_sampler,
                material_set,
                4);
            mt_render.cmd_bind_image(
                cb,
                primitive->material->emissive_image,
                primitive->material->emissive_sampler,
                material_set,
                5);

            if (primitive->has_indices)
            {
                mt_render.cmd_draw_indexed(
                    cb, primitive->index_count, 1, primitive->first_index, 0, 0);
            }
            else
            {
                mt_render.cmd_draw(cb, primitive->vertex_count, 1, 0, 0);
            }
        }
    }
    for (GltfNode **child = node->children; child != node->children + mt_array_size(node->children);
         ++child)
    {
        node_draw(*child, cb, transform, model_set, material_set);
    }
}

void mt_gltf_asset_draw(
    MtGltfAsset *asset, MtCmdBuffer *cb, Mat4 *transform, uint32_t model_set, uint32_t material_set)
{
    mt_render.cmd_bind_vertex_buffer(cb, asset->vertex_buffer, 0);
    mt_render.cmd_bind_index_buffer(cb, asset->index_buffer, MT_INDEX_TYPE_UINT32, 0);
    for (GltfNode **node = asset->nodes; node != asset->nodes + mt_array_size(asset->nodes); ++node)
    {
        node_draw(*node, cb, transform, model_set, material_set);
    }
}

static const char *g_extensions[] = {
    ".gltf",
    ".glb",
};

static MtAssetVT g_asset_vt = {
    .name            = "GLTF model",
    .extensions      = g_extensions,
    .extension_count = MT_LENGTH(g_extensions),
    .size            = sizeof(MtGltfAsset),
    .init            = asset_init,
    .destroy         = asset_destroy,
};
MtAssetVT *mt_gltf_asset_vt = &g_asset_vt;
