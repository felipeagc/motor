#include <motor/engine/environment.h>

#include <string.h>
#include <motor/engine/engine.h>
#include <motor/engine/asset_manager.h>
#include <motor/engine/assets/pipeline_asset.h>
#include <motor/engine/assets/image_asset.h>
#include <motor/graphics/renderer.h>

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

    env->radiance_sampler = mt_render.create_sampler(
        engine->device,
        &(MtSamplerCreateInfo){
            .anisotropy = true,
            .mag_filter = MT_FILTER_LINEAR,
            .min_filter = MT_FILTER_LINEAR,
        });

    env->uniform.sun_direction = V3(0.0f, -1.0f, 0.0f);
    env->uniform.exposure      = 8.0f;

    env->uniform.sun_color     = V3(1.0f, 1.0f, 1.0f);
    env->uniform.sun_intensity = 1.0f;

    env->uniform.radiance_mip_levels = 1;
    env->uniform.point_light_count   = 0;
}

void mt_environment_draw_skybox(MtEnvironment *env, MtCmdBuffer *cb)
{
    MtEngine *engine = env->asset_manager->engine;

    mt_render.cmd_bind_pipeline(cb, env->skybox_pipeline->pipeline);

    mt_render.cmd_bind_uniform(cb, &env->uniform, sizeof(env->uniform), 1, 0);
    if (env->skybox_asset)
    {
        mt_render.cmd_bind_image(cb, env->skybox_asset->image, env->regular_sampler, 1, 1);
    }
    else
    {
        mt_render.cmd_bind_image(cb, engine->black_image, env->regular_sampler, 1, 1);
    }

    mt_render.cmd_draw(cb, 36, 1, 0, 0);
}

void mt_environment_destroy(MtEnvironment *env)
{
    MtEngine *engine = env->asset_manager->engine;
    mt_render.destroy_sampler(engine->device, env->regular_sampler);
    mt_render.destroy_sampler(engine->device, env->radiance_sampler);
}
