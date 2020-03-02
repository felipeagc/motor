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
#include "common_geometry.inl"

typedef enum CubemapType {
    CUBEMAP_IRRADIANCE,
    CUBEMAP_RADIANCE,
} CubemapType;

// BRDF LUT {{{
typedef struct BRDFGraphData
{
    uint32_t dim;
} BRDFGraphData;

static void brdf_graph_builder(MtRenderGraph *graph, void *user_data)
{
    BRDFGraphData *data = user_data;

    MtRenderGraphImage brdf_info = {
        .format = MT_FORMAT_RG16_SFLOAT, .width = data->dim, .height = data->dim};
    mt_render.graph_add_image(graph, "brdf", &brdf_info);

    MtRenderGraphPass *pass =
        mt_render.graph_add_pass(graph, "brdf_pass", MT_PIPELINE_STAGE_ALL_GRAPHICS);
    mt_render.pass_write(pass, MT_PASS_WRITE_COLOR_ATTACHMENT, "brdf");
}

static MtImage *generate_brdf_lut(MtEngine *engine)
{
    MtPipeline *pipeline = engine->brdf_pipeline->pipeline;

    // Create image
    const uint32_t dim = 512;

    BRDFGraphData data = {.dim = dim};

    MtRenderGraph *graph = mt_render.create_graph(engine->device, NULL, false);
    mt_render.graph_set_user_data(graph, &data);
    mt_render.graph_set_builder(graph, brdf_graph_builder);

    {
        MtCmdBuffer *cb = mt_render.pass_begin(graph, "brdf_pass");

        mt_render.cmd_bind_pipeline(cb, pipeline);
        mt_render.cmd_draw(cb, 3, 1, 0, 0);

        mt_render.pass_end(graph, "brdf_pass");
    }

    mt_render.graph_execute(graph);
    mt_render.graph_wait_all(graph);

    MtImage *brdf = mt_render.graph_consume_image(graph, "brdf");
    mt_render.destroy_graph(graph);

    return brdf;
}
// }}}

// Cubemap generation {{{
typedef struct CubemapGraphData
{
    uint32_t dim;
    uint32_t mip_count;
    MtFormat format;
} CubemapGraphData;

static void cubemap_graph_builder(MtRenderGraph *graph, void *user_data)
{
    CubemapGraphData *data = user_data;

    MtRenderGraphImage color_info = {
        .width = data->dim, .height = data->dim, .format = data->format};
    MtRenderGraphImage cube_info = {
        .width = data->dim,
        .height = data->dim,
        .format = data->format,
        .layer_count = 6,
        .mip_count = data->mip_count,
    };

    mt_render.graph_add_image(graph, "offscreen", &color_info);
    mt_render.graph_add_image(graph, "cubemap", &cube_info);

    MtRenderGraphPass *layer_pass =
        mt_render.graph_add_pass(graph, "layer_pass", MT_PIPELINE_STAGE_ALL_GRAPHICS);
    mt_render.pass_write(layer_pass, MT_PASS_WRITE_COLOR_ATTACHMENT, "offscreen");

    MtRenderGraphPass *transfer_pass =
        mt_render.graph_add_pass(graph, "transfer_pass", MT_PIPELINE_STAGE_TRANSFER);
    mt_render.pass_read(transfer_pass, MT_PASS_READ_IMAGE_TRANSFER, "offscreen");
    mt_render.pass_write(transfer_pass, MT_PASS_WRITE_IMAGE_TRANSFER, "cubemap");
}

static MtImage *generate_cubemap(MtEnvironment *env, CubemapType type)
{
    MtEngine *engine = env->engine;

    MtPipeline *pipeline = NULL;
    switch (type)
    {
        case CUBEMAP_IRRADIANCE: pipeline = env->engine->irradiance_pipeline->pipeline; break;
        case CUBEMAP_RADIANCE: pipeline = env->engine->prefilter_env_pipeline->pipeline; break;
    }

    // Create image
    uint32_t dim;
    MtFormat format;

    switch (type)
    {
        case CUBEMAP_IRRADIANCE:
            dim = 64;
            format = MT_FORMAT_RGBA32_SFLOAT;
            break;
        case CUBEMAP_RADIANCE:
            dim = 512;
            format = MT_FORMAT_RGBA16_SFLOAT;
            break;
    }

    uint32_t mip_count = (uint32_t)(floor(log2(dim))) + 1;

    if (type == CUBEMAP_RADIANCE)
    {
        env->radiance_mip_levels = (float)mip_count;
    }

    CubemapGraphData data = {.dim = dim, .mip_count = mip_count, .format = format};

    MtRenderGraph *graph = mt_render.create_graph(engine->device, NULL, false);
    mt_render.graph_set_user_data(graph, &data);
    mt_render.graph_set_builder(graph, cubemap_graph_builder);

    for (uint32_t m = 0; m < mip_count; m++)
    {
        for (uint32_t f = 0; f < 6; f++)
        {
            {
                MtCmdBuffer *cb = mt_render.pass_begin(graph, "layer_pass");

                struct
                {
                    Mat4 mvp;
                    float roughness;
                } uniform;

                uniform.mvp = mat4_mul(
                    g_direction_matrices[f],
                    mat4_perspective(((float)MT_PI / 2.0f), 1.0f, 0.1f, 512.0f));

                if (type == CUBEMAP_RADIANCE)
                {
                    uniform.roughness = (float)m / (float)(mip_count - 1);
                }

                float mip_dim = (float)dim * powf(0.5f, (float)m);

                mt_render.cmd_set_viewport(
                    cb,
                    &(MtViewport){
                        .x = 0.0f,
                        .y = 0.0f,
                        .width = mip_dim,
                        .height = mip_dim,
                        .min_depth = 0.0f,
                        .max_depth = 1.0f,
                    });

                mt_render.cmd_set_scissor(cb, 0, 0, dim, dim);

                mt_render.cmd_bind_pipeline(cb, pipeline);
                mt_render.cmd_bind_uniform(cb, &uniform, sizeof(uniform), 0, 0);
                mt_render.cmd_bind_sampler(cb, env->skybox_sampler, 0, 1);
                mt_render.cmd_bind_image(cb, env->skybox_image, 0, 2);
                void *mapping = mt_render.cmd_bind_vertex_data(cb, sizeof(g_cube_vertices));
                memcpy(mapping, g_cube_vertices, sizeof(g_cube_vertices));
                mt_render.cmd_draw(cb, 36, 1, 0, 0);

                mt_render.pass_end(graph, "layer_pass");
            }

            {
                MtCmdBuffer *cb = mt_render.pass_begin(graph, "transfer_pass");

                MtImage *offscreen = mt_render.graph_get_image(graph, "offscreen");
                MtImage *cubemap = mt_render.graph_get_image(graph, "cubemap");

                float mip_dim = (float)dim * powf(0.5f, (float)m);

                mt_render.cmd_copy_image_to_image(
                    cb,
                    &(MtImageCopyView){
                        .image = offscreen,
                        .mip_level = 0,
                        .array_layer = 0,
                    },
                    &(MtImageCopyView){
                        .image = cubemap,
                        .mip_level = m,
                        .array_layer = f,
                    },
                    (MtExtent3D){
                        .width = (uint32_t)mip_dim,
                        .height = (uint32_t)mip_dim,
                        .depth = 1,
                    });

                mt_render.pass_end(graph, "transfer_pass");
            }

            mt_render.graph_execute(graph);
        }
    }

    mt_render.graph_wait_all(graph);

    MtImage *cubemap = mt_render.graph_consume_image(graph, "cubemap");

    mt_render.destroy_graph(graph);

    return cubemap;
}
// }}}

static void maybe_generate_images(MtEnvironment *env)
{
    MtEngine *engine = env->engine;

    MtImage *old_skybox = env->skybox_image;
    MtImage *old_irradiance = env->irradiance_image;
    MtImage *old_radiance = env->radiance_image;

    env->skybox_image = (env->skybox_asset) ? env->skybox_asset->image : engine->default_cubemap;

    if (env->skybox_image != old_skybox)
    {
        env->irradiance_image = generate_cubemap(env, CUBEMAP_IRRADIANCE);
        mt_log_debug("Generated irradiance cubemap");

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
                .max_lod = env->radiance_mip_levels,
            });
    }
}

void mt_environment_init(MtEnvironment *env, MtEngine *engine)
{
    memset(env, 0, sizeof(*env));

    env->engine = engine;

    env->radiance_mip_levels = 1.0f;

    env->brdf_image = generate_brdf_lut(engine);

    env->uniform.sun_direction = V3(1.0f, -1.0f, -1.0f);
    env->uniform.exposure = 4.5f;

    env->uniform.sun_color = V3(1.0f, 1.0f, 1.0f);
    env->uniform.sun_intensity = 1.0f;

    env->uniform.point_light_count = 0;

    env->skybox_sampler = mt_render.create_sampler(
        engine->device,
        &(MtSamplerCreateInfo){
            .anisotropy = false,
            .mag_filter = MT_FILTER_LINEAR,
            .min_filter = MT_FILTER_LINEAR,
            .max_lod = 1.0f,
            .address_mode = MT_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
            .border_color = MT_BORDER_COLOR_FLOAT_OPAQUE_WHITE,
        });

    maybe_generate_images(env);
}

void mt_environment_set_skybox(MtEnvironment *env, MtImageAsset *skybox_asset)
{
    env->skybox_asset = skybox_asset;
}

void mt_environment_draw_skybox(MtEnvironment *env, MtCmdBuffer *cb, const MtCameraUniform *cam)
{
    maybe_generate_images(env);

    mt_render.cmd_bind_pipeline(cb, env->engine->skybox_pipeline->pipeline);

    mt_render.cmd_bind_uniform(cb, cam, sizeof(*cam), 0, 0);
    mt_render.cmd_bind_sampler(cb, env->radiance_sampler, 1, 0);
    mt_render.cmd_bind_image(cb, env->radiance_image, 1, 1);
    mt_render.cmd_bind_uniform(cb, &env->uniform, sizeof(env->uniform), 1, 2);
    void *mapping = mt_render.cmd_bind_vertex_data(cb, sizeof(g_cube_vertices));
    memcpy(mapping, g_cube_vertices, sizeof(g_cube_vertices));

    mt_render.cmd_draw(cb, 36, 1, 0, 0);
}

void mt_environment_bind(MtEnvironment *env, MtCmdBuffer *cb, uint32_t set)
{
    maybe_generate_images(env);

    mt_render.cmd_bind_uniform(cb, &env->uniform, sizeof(env->uniform), set, 0);
    mt_render.cmd_bind_sampler(cb, env->skybox_sampler, set, 1);
    mt_render.cmd_bind_sampler(cb, env->radiance_sampler, set, 2);
    mt_render.cmd_bind_image(cb, env->irradiance_image, set, 3);
    mt_render.cmd_bind_image(cb, env->radiance_image, set, 4);
    mt_render.cmd_bind_image(cb, env->brdf_image, set, 5);
}

void mt_environment_destroy(MtEnvironment *env)
{
    MtEngine *engine = env->engine;

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
