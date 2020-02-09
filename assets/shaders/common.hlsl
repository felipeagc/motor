#ifndef COMMON_HLSL
#define COMMON_HLSL

#ifndef PI
#define PI 3.14159265359
#endif

#define MAX_POINT_LIGHTS 64
#define TILE_SIZE 16

struct PointLight
{
    float4 pos;
    float3 color;
    float radius;
};

struct Camera
{
    float4x4 view;
    float4x4 proj;
    float4 pos;
};

struct Material
{
    float4 base_color;
    float metallic;
    float roughness;
    float4 emissive;
};

struct Environment
{
    float3 sun_direction;
    float exposure;

    float3 sun_color;
    float sun_intensity;

    float4x4 light_space_matrix;

    uint light_count;

    float pad1;
    float pad2;
    float pad3;

    PointLight point_lights[MAX_POINT_LIGHTS];
};

struct Model
{
    float4x4 local_model;
    float4x4 model;
    float normal_mapped;
};

#endif
