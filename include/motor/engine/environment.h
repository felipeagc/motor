#pragma once

#include <stdint.h>
#include <motor/base/math_types.h>

typedef struct MtCmdBuffer MtCmdBuffer;
typedef struct MtImage MtImage;
typedef struct MtSampler MtSampler;
typedef struct MtAssetManager MtAssetManager;
typedef struct MtImageAsset MtImageAsset;
typedef struct MtPipelineAsset MtPipelineAsset;

#define MT_MAX_POINT_LIGHTS 20

typedef struct MtPointLight
{
    Vec4 pos;
    Vec4 color;
} MtPointLight;

typedef struct MtEnvironmentUniform
{
    Vec3 sun_direction;
    float exposure;

    Vec3 sun_color;
    float sun_intensity;

    Mat4 light_space_matrix;

    float radiance_mip_levels;
    uint32_t point_light_count;

    float dummy1;
    float dummy2;

    MtPointLight point_lights[MT_MAX_POINT_LIGHTS];
} MtEnvironmentUniform;

typedef struct MtEnvironment
{
    MtAssetManager *asset_manager;

    MtPipelineAsset *skybox_pipeline;

    MtImageAsset *skybox_asset;

    MtImage *skybox_image;
    MtImage *irradiance_image;
    MtImage *radiance_image;

    MtImage *brdf_image;

    MtSampler *radiance_sampler;

    MtEnvironmentUniform uniform;
} MtEnvironment;

void mt_environment_init(
    MtEnvironment *env, MtAssetManager *asset_manager, MtImageAsset *skybox_asset);

void mt_environment_draw_skybox(MtEnvironment *env, MtCmdBuffer *cb);

void mt_environment_bind(MtEnvironment *env, MtCmdBuffer *cb, uint32_t set);

void mt_environment_destroy(MtEnvironment *env);
