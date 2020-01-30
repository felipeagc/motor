#include <motor/engine/environment.h>

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <motor/base/log.h>
#include <motor/base/allocator.h>
#include <motor/base/math.h>
#include <motor/engine/engine.h>
#include <motor/engine/asset_manager.h>
#include <motor/engine/assets/pipeline_asset.h>
#include <motor/engine/assets/image_asset.h>
#include <motor/graphics/renderer.h>
#include "pipeline_utils.inl"

typedef enum CubemapType
{
    CUBEMAP_IRRADIANCE,
    CUBEMAP_RADIANCE,
} CubemapType;

static const Vec3 cube_positions[36] = {
    {-0.5, 0.5, -0.5},  {-0.5, -0.5, -0.5}, {0.5, -0.5, -0.5},
    {0.5, -0.5, -0.5},  {0.5, 0.5, -0.5},   {-0.5, 0.5, -0.5},

    {-0.5, -0.5, 0.5},  {-0.5, -0.5, -0.5}, {-0.5, 0.5, -0.5},
    {-0.5, 0.5, -0.5},  {-0.5, 0.5, 0.5},   {-0.5, -0.5, 0.5},

    {0.5, -0.5, -0.5},  {0.5, -0.5, 0.5},   {0.5, 0.5, 0.5},
    {0.5, 0.5, 0.5},    {0.5, 0.5, -0.5},   {0.5, -0.5, -0.5},

    {-0.5, -0.5, 0.5},  {-0.5, 0.5, 0.5},   {0.5, 0.5, 0.5},
    {0.5, 0.5, 0.5},    {0.5, -0.5, 0.5},   {-0.5, -0.5, 0.5},

    {-0.5, 0.5, -0.5},  {0.5, 0.5, -0.5},   {0.5, 0.5, 0.5},
    {0.5, 0.5, 0.5},    {-0.5, 0.5, 0.5},   {-0.5, 0.5, -0.5},

    {-0.5, -0.5, -0.5}, {-0.5, -0.5, 0.5},  {0.5, -0.5, -0.5},
    {0.5, -0.5, -0.5},  {-0.5, -0.5, 0.5},  {0.5, -0.5, 0.5},
};

static const Mat4 direction_matrices[6] = {
    {{
        {0.0, 0.0, -1.0, 0.0},
        {0.0, -1.0, 0.0, 0.0},
        {-1.0, 0.0, 0.0, 0.0},
        {0.0, 0.0, 0.0, 1.0},
    }},
    {{
        {0.0, 0.0, 1.0, 0.0},
        {0.0, -1.0, 0.0, 0.0},
        {1.0, 0.0, 0.0, 0.0},
        {0.0, 0.0, 0.0, 1.0},
    }},
    {{
        {1.0, 0.0, 0.0, 0.0},
        {0.0, 0.0, -1.0, 0.0},
        {0.0, 1.0, 0.0, 0.0},
        {0.0, 0.0, 0.0, 1.0},
    }},
    {{
        {1.0, 0.0, 0.0, 0.0},
        {0.0, 0.0, 1.0, 0.0},
        {0.0, -1.0, 0.0, 0.0},
        {0.0, 0.0, 0.0, 1.0},
    }},
    {{
        {1.0, 0.0, 0.0, 0.0},
        {0.0, -1.0, 0.0, 0.0},
        {0.0, 0.0, -1.0, 0.0},
        {0.0, 0.0, 0.0, 1.0},
    }},
    {{
        {-1.0, 0.0, 0.0, 0.0},
        {0.0, -1.0, 0.0, 0.0},
        {0.0, 0.0, 1.0, 0.0},
        {0.0, 0.0, 0.0, 1.0},
    }},
};

// BRDF LUT {{{
static MtImage *generate_brdf_lut(MtEngine *engine)
{
    // Create pipeline
    const char *path = "../assets/shaders/brdf.glsl";

    FILE *f = fopen(path, "rb");
    if (!f)
    {
        mt_log_error("Failed to open file: %s", path);
        return NULL;
    }

    fseek(f, 0, SEEK_END);
    size_t input_size = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *input = mt_alloc(engine->alloc, input_size);
    fread(input, input_size, 1, f);
    fclose(f);

    MtPipeline *pipeline = create_pipeline(engine, "", input, input_size);

    mt_free(engine->alloc, input);

    // Create image
    const uint32_t dim = 512;

    MtImage *brdf = mt_render.create_image(
        engine->device,
        &(MtImageCreateInfo){
            .width  = dim,
            .height = dim,
            .format = MT_FORMAT_RG16_SFLOAT,
            .usage  = MT_IMAGE_USAGE_SAMPLED_BIT | MT_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        });

    MtRenderPass *rp = mt_render.create_render_pass(
        engine->device,
        &(MtRenderPassCreateInfo){
            .color_attachments = &brdf,
        });

    MtFence *fence = mt_render.create_fence(engine->device);

    MtCmdBuffer *cb;
    mt_render.allocate_cmd_buffers(engine->device, MT_QUEUE_GRAPHICS, 1, &cb);

    {
        mt_render.begin_cmd_buffer(cb);

        mt_render.cmd_begin_render_pass(cb, rp, NULL, NULL);

        mt_render.cmd_bind_pipeline(cb, pipeline);
        mt_render.cmd_draw(cb, 3, 1, 0, 0);

        mt_render.cmd_end_render_pass(cb);

        mt_render.end_cmd_buffer(cb);
    }

    mt_render.submit(engine->device, cb, fence);
    mt_render.wait_for_fence(engine->device, fence);

    mt_render.destroy_fence(engine->device, fence);
    mt_render.free_cmd_buffers(engine->device, MT_QUEUE_GRAPHICS, 1, &cb);

    mt_render.destroy_render_pass(engine->device, rp);
    mt_render.destroy_pipeline(engine->device, pipeline);

    return brdf;
}
// }}}

// Cubemap generation {{{
static MtImage *generate_cubemap(MtEnvironment *env, CubemapType type)
{
    MtEngine *engine = env->asset_manager->engine;

    const char *path = NULL;
    switch (type)
    {
        case CUBEMAP_IRRADIANCE: path = "../assets/shaders/irradiance_cube.glsl"; break;
        case CUBEMAP_RADIANCE: path = "../assets/shaders/prefilter_env_map.glsl"; break;
    }

    FILE *f = fopen(path, "rb");
    if (!f)
    {
        mt_log_error("Failed to open file: %s", path);
        return NULL;
    }

    fseek(f, 0, SEEK_END);
    size_t input_size = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *input = mt_alloc(engine->alloc, input_size);
    fread(input, input_size, 1, f);
    fclose(f);

    MtPipeline *pipeline = create_pipeline(engine, "", input, input_size);

    mt_free(engine->alloc, input);

    // Create image
    uint32_t dim;
    MtFormat format;

    switch (type)
    {
        case CUBEMAP_IRRADIANCE:
            dim    = 64;
            format = MT_FORMAT_RGBA32_SFLOAT;
            break;
        case CUBEMAP_RADIANCE:
            dim    = 512;
            format = MT_FORMAT_RGBA16_SFLOAT;
            break;
    }

    uint32_t mip_count = (uint32_t)(floor(log2(dim))) + 1;

    if (type == CUBEMAP_RADIANCE)
    {
        env->uniform.radiance_mip_levels = (float)mip_count;
    }

    MtFence *fence = mt_render.create_fence(engine->device);

    MtCmdBuffer *cb;
    mt_render.allocate_cmd_buffers(engine->device, MT_QUEUE_GRAPHICS, 1, &cb);

    MtImage *cubemap = mt_render.create_image(
        engine->device,
        &(MtImageCreateInfo){
            .width       = dim,
            .height      = dim,
            .format      = format,
            .usage       = MT_IMAGE_USAGE_SAMPLED_BIT | MT_IMAGE_USAGE_TRANSFER_DST_BIT,
            .layer_count = 6,
            .mip_count   = mip_count,
        });

    MtImage *offscreen = mt_render.create_image(
        engine->device,
        &(MtImageCreateInfo){
            .width  = dim,
            .height = dim,
            .format = format,
            .usage  = MT_IMAGE_USAGE_SAMPLED_BIT | MT_IMAGE_USAGE_TRANSFER_SRC_BIT |
                     MT_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        });

    MtRenderPass *rp = mt_render.create_render_pass(
        engine->device,
        &(MtRenderPassCreateInfo){
            .color_attachments = &offscreen,
        });

    struct
    {
        Mat4 mvp;
        float roughness;
    } uniform;

    {
        mt_render.begin_cmd_buffer(cb);

        mt_render.cmd_pipeline_image_barrier(
            cb,
            &(MtImageBarrier){
                .image       = cubemap,
                .old_layout  = MT_IMAGE_LAYOUT_UNDEFINED,
                .new_layout  = MT_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                .level_count = mip_count,
                .layer_count = 6,
            });

        mt_render.end_cmd_buffer(cb);

        mt_render.submit(engine->device, cb, fence);
        mt_render.wait_for_fence(engine->device, fence);
        mt_render.reset_fence(engine->device, fence);
    }

    for (uint32_t m = 0; m < mip_count; m++)
    {
        for (uint32_t f = 0; f < 6; f++)
        {
            uniform.mvp = mat4_mul(
                direction_matrices[f], mat4_perspective(((float)MT_PI / 2.0f), 1.0f, 0.1f, 512.0f));

            if (type == CUBEMAP_RADIANCE)
            {
                uniform.roughness = (float)m / (float)(mip_count - 1);
            }

            mt_render.begin_cmd_buffer(cb);

            mt_render.cmd_begin_render_pass(cb, rp, NULL, NULL);

            float mip_dim = (float)dim * powf(0.5f, (float)m);

            mt_render.cmd_set_viewport(
                cb,
                &(MtViewport){
                    .x         = 0.0f,
                    .y         = 0.0f,
                    .width     = mip_dim,
                    .height    = mip_dim,
                    .min_depth = 0.0f,
                    .max_depth = 1.0f,
                });

            mt_render.cmd_set_scissor(cb, 0, 0, dim, dim);

            mt_render.cmd_bind_pipeline(cb, pipeline);
            mt_render.cmd_bind_uniform(cb, &uniform, sizeof(uniform), 0, 0);
            mt_render.cmd_bind_image(cb, env->skybox_image, env->skybox_sampler, 0, 1);
            mt_render.cmd_bind_vertex_data(cb, cube_positions, sizeof(cube_positions));
            mt_render.cmd_draw(cb, 36, 1, 0, 0);

            mt_render.cmd_end_render_pass(cb);

            mt_render.cmd_pipeline_image_barrier(
                cb,
                &(MtImageBarrier){
                    .image      = offscreen,
                    .old_layout = MT_IMAGE_LAYOUT_UNDEFINED,
                    .new_layout = MT_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                });

            mt_render.cmd_copy_image_to_image(
                cb,
                &(MtImageCopyView){
                    .image       = offscreen,
                    .mip_level   = 0,
                    .array_layer = 0,
                },
                &(MtImageCopyView){
                    .image       = cubemap,
                    .mip_level   = m,
                    .array_layer = f,
                },
                (MtExtent3D){
                    .width  = (uint32_t)mip_dim,
                    .height = (uint32_t)mip_dim,
                    .depth  = 1,
                });

            mt_render.cmd_pipeline_image_barrier(
                cb,
                &(MtImageBarrier){
                    .image      = offscreen,
                    .old_layout = MT_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                    .new_layout = MT_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                });

            mt_render.end_cmd_buffer(cb);

            mt_render.submit(engine->device, cb, fence);
            mt_render.wait_for_fence(engine->device, fence);
            mt_render.reset_fence(engine->device, fence);
        }
    }

    {
        mt_render.begin_cmd_buffer(cb);

        mt_render.cmd_pipeline_image_barrier(
            cb,
            &(MtImageBarrier){
                .image       = cubemap,
                .old_layout  = MT_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                .new_layout  = MT_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                .level_count = mip_count,
                .layer_count = 6,
            });

        mt_render.end_cmd_buffer(cb);

        mt_render.submit(engine->device, cb, fence);
        mt_render.wait_for_fence(engine->device, fence);
        mt_render.reset_fence(engine->device, fence);
    }

    mt_render.destroy_fence(engine->device, fence);
    mt_render.free_cmd_buffers(engine->device, MT_QUEUE_GRAPHICS, 1, &cb);

    mt_render.destroy_render_pass(engine->device, rp);
    mt_render.destroy_image(engine->device, offscreen);
    mt_render.destroy_pipeline(engine->device, pipeline);

    return cubemap;
}
// }}}

static void maybe_generate_images(MtEnvironment *env)
{
    MtEngine *engine = env->asset_manager->engine;

    MtImage *old_skybox     = env->skybox_image;
    MtImage *old_irradiance = env->irradiance_image;
    MtImage *old_radiance   = env->radiance_image;

    env->skybox_image = (env->skybox_asset) ? env->skybox_asset->image : engine->default_cubemap;

    if (env->skybox_image != old_skybox)
    {
        if (old_skybox)
        {
            mt_render.destroy_image(engine->device, old_skybox);
        }

        mt_log_debug("Generating irradiance cubemap");
        env->irradiance_image = generate_cubemap(env, CUBEMAP_IRRADIANCE);
        mt_log_debug("Generated irradiance cubemap");

        mt_log_debug("Generating radiance cubemap");
        env->radiance_image = generate_cubemap(env, CUBEMAP_RADIANCE);
        mt_log_debug("Generated radiance cubemap");
    }

    if (env->irradiance_image != old_irradiance)
    {
        if (old_irradiance)
        {
            mt_render.destroy_image(engine->device, old_irradiance);
        }
    }

    if (env->radiance_image != old_radiance)
    {
        if (old_radiance)
        {
            mt_render.destroy_image(engine->device, old_radiance);
        }

        if (env->radiance_sampler)
        {
            mt_render.destroy_sampler(engine->device, env->radiance_sampler);
        }

        env->radiance_sampler = mt_render.create_sampler(
            engine->device,
            &(MtSamplerCreateInfo){
                .anisotropy = false,
                .mag_filter = MT_FILTER_LINEAR,
                .min_filter = MT_FILTER_LINEAR,
                .max_lod    = env->uniform.radiance_mip_levels,
            });
    }
}

void mt_environment_init(
    MtEnvironment *env, MtAssetManager *asset_manager, MtImageAsset *skybox_asset)
{
    memset(env, 0, sizeof(*env));

    env->asset_manager = asset_manager;

    MtEngine *engine = env->asset_manager->engine;

    env->skybox_pipeline =
        (MtPipelineAsset *)mt_asset_manager_load(asset_manager, "../assets/shaders/skybox.glsl");

    env->skybox_asset = skybox_asset;

    env->brdf_image = generate_brdf_lut(engine);

    env->uniform.sun_direction = V3(1.0f, -1.0f, -1.0f);
    env->uniform.exposure      = 4.5f;

    env->uniform.sun_color     = V3(1.0f, 1.0f, 1.0f);
    env->uniform.sun_intensity = 1.0f;

    env->uniform.radiance_mip_levels = 1.0f;
    env->uniform.point_light_count   = 0;

    env->skybox_sampler = mt_render.create_sampler(
        engine->device,
        &(MtSamplerCreateInfo){
            .anisotropy   = false,
            .mag_filter   = MT_FILTER_LINEAR,
            .min_filter   = MT_FILTER_LINEAR,
            .max_lod      = 1.0f,
            .address_mode = MT_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
            .border_color = MT_BORDER_COLOR_FLOAT_OPAQUE_WHITE,
        });

    maybe_generate_images(env);
}

void mt_environment_draw_skybox(MtEnvironment *env, MtCmdBuffer *cb)
{
    maybe_generate_images(env);

    mt_render.cmd_bind_pipeline(cb, env->skybox_pipeline->pipeline);

    mt_render.cmd_bind_image(cb, env->radiance_image, env->skybox_sampler, 1, 0);
    mt_render.cmd_bind_uniform(cb, &env->uniform, sizeof(env->uniform), 1, 1);
    mt_render.cmd_bind_vertex_data(cb, cube_positions, sizeof(cube_positions));

    mt_render.cmd_draw(cb, 36, 1, 0, 0);
}

void mt_environment_bind(MtEnvironment *env, MtCmdBuffer *cb, uint32_t set)
{
    mt_render.cmd_bind_uniform(cb, &env->uniform, sizeof(env->uniform), set, 0);
    mt_render.cmd_bind_image(cb, env->irradiance_image, env->skybox_sampler, set, 1);
    mt_render.cmd_bind_image(cb, env->radiance_image, env->radiance_sampler, set, 2);
    mt_render.cmd_bind_image(cb, env->brdf_image, env->skybox_sampler, set, 3);
}

void mt_environment_destroy(MtEnvironment *env)
{
    MtEngine *engine = env->asset_manager->engine;

    if (env->irradiance_image)
    {
        mt_render.destroy_image(engine->device, env->irradiance_image);
    }

    if (env->radiance_image)
    {
        mt_render.destroy_image(engine->device, env->radiance_image);
    }

    if (env->radiance_sampler)
    {
        mt_render.destroy_sampler(engine->device, env->radiance_sampler);
    }

    mt_render.destroy_sampler(engine->device, env->skybox_sampler);
    mt_render.destroy_image(engine->device, env->brdf_image);
}
