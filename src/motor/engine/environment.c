#include <motor/engine/environment.h>

#include <stdio.h>
#include <string.h>
#include <motor/base/allocator.h>
#include <motor/engine/engine.h>
#include <motor/engine/asset_manager.h>
#include <motor/engine/assets/pipeline_asset.h>
#include <motor/engine/assets/image_asset.h>
#include <motor/graphics/renderer.h>
#include "pipeline_utils.inl"

static MtImage *generate_brdf_lut(MtEngine *engine)
{
    // Create pipeline
    const char *path = "../assets/shaders/brdf.glsl";

    FILE *f = fopen(path, "rb");
    if (!f)
    {
        printf("Failed to open file: %s\n", path);
        return false;
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
            .color_attachment = brdf,
        });

    MtFence *fence = mt_render.create_fence(engine->device);

    MtCmdBuffer *cb;
    mt_render.allocate_cmd_buffers(engine->device, MT_QUEUE_GRAPHICS, 1, &cb);

    {
        mt_render.begin_cmd_buffer(cb);

        mt_render.cmd_begin_render_pass(cb, rp);

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

static void maybe_generate_images(MtEnvironment *env)
{
    MtEngine *engine = env->asset_manager->engine;

    if (env->irradiance_asset)
    {
        if (env->irradiance_image != env->irradiance_asset->image)
        {
            env->irradiance_image = env->irradiance_asset->image;
        }
    }
    else
    {
        env->irradiance_image = env->asset_manager->engine->default_cubemap;
    }

    if (env->radiance_asset)
    {
        if (env->radiance_image != env->radiance_asset->image)
        {
            env->radiance_image = env->radiance_asset->image;
        }
    }
    else
    {
        env->radiance_image              = env->asset_manager->engine->default_cubemap;
        env->uniform.radiance_mip_levels = 1.0f;
    }

    if (env->skybox_asset)
    {
        if (env->skybox_image != env->skybox_asset->image)
        {
            env->skybox_image = env->skybox_asset->image;

            if (env->irradiance_asset == NULL)
            {
                // TODO: Generate image
            }

            if (env->radiance_asset == NULL)
            {
                // TODO: Generate image
            }
        }
    }
    else
    {
        env->skybox_image = env->asset_manager->engine->default_cubemap;
    }

    if (env->radiance_sampler)
    {
        mt_render.destroy_sampler(engine->device, env->radiance_sampler);
    }

    env->radiance_sampler = mt_render.create_sampler(
        engine->device,
        &(MtSamplerCreateInfo){
            .anisotropy = true,
            .mag_filter = MT_FILTER_LINEAR,
            .min_filter = MT_FILTER_LINEAR,
            .max_lod    = env->uniform.radiance_mip_levels,
        });
}

void mt_environment_init(
    MtEnvironment *env,
    MtAssetManager *asset_manager,
    MtImageAsset *skybox_asset,
    MtImageAsset *irradiance_asset,
    MtImageAsset *radiance_asset)
{
    memset(env, 0, sizeof(*env));

    env->asset_manager = asset_manager;

    MtEngine *engine = env->asset_manager->engine;

    env->skybox_pipeline =
        (MtPipelineAsset *)mt_asset_manager_load(asset_manager, "../assets/shaders/skybox.glsl");

    env->skybox_asset     = skybox_asset;
    env->irradiance_asset = irradiance_asset;
    env->radiance_asset   = radiance_asset;

    env->regular_sampler = mt_render.create_sampler(
        engine->device,
        &(MtSamplerCreateInfo){
            .anisotropy = true,
            .mag_filter = MT_FILTER_LINEAR,
            .min_filter = MT_FILTER_LINEAR,
        });

    env->brdf_image = generate_brdf_lut(engine);

    env->uniform.sun_direction = V3(0.0f, -1.0f, 0.0f);
    env->uniform.exposure      = 8.0f;

    env->uniform.sun_color     = V3(1.0f, 1.0f, 1.0f);
    env->uniform.sun_intensity = 1.0f;

    env->uniform.radiance_mip_levels = 1.0f;
    env->uniform.point_light_count   = 0;

    maybe_generate_images(env);
}

void mt_environment_draw_skybox(MtEnvironment *env, MtCmdBuffer *cb)
{
    MtEngine *engine = env->asset_manager->engine;

    maybe_generate_images(env);

    mt_render.cmd_bind_pipeline(cb, env->skybox_pipeline->pipeline);

    mt_render.cmd_bind_uniform(cb, &env->uniform, sizeof(env->uniform), 1, 0);
    mt_render.cmd_bind_image(cb, env->skybox_image, env->regular_sampler, 1, 1);

    mt_render.cmd_draw(cb, 36, 1, 0, 0);
}

void mt_environment_destroy(MtEnvironment *env)
{
    MtEngine *engine = env->asset_manager->engine;
    mt_render.destroy_image(engine->device, env->brdf_image);
    mt_render.destroy_sampler(engine->device, env->regular_sampler);
    mt_render.destroy_sampler(engine->device, env->radiance_sampler);
}
