#ifndef COMMON_GLSL
#define COMMON_GLSL

#ifndef PI
#define PI 3.14159265359
#endif

#define MAX_POINT_LIGHTS 64

struct PointLight {
    vec4 pos;
    vec4 color;
};

struct Camera {
    mat4 view;
    mat4 proj;
    vec4 pos;
};

struct Material {
    vec4 base_color;
    float metallic;
    float roughness;
    vec4 emissive;
};

struct Environment {
    vec3 sun_direction;
    float exposure;

    vec3 sun_color;
    float sun_intensity;

    mat4 light_space_matrix;

    float radiance_mip_levels;
    uint light_count;

    float dummy1;
    float dummy2;

    PointLight point_lights[MAX_POINT_LIGHTS];
};

struct Model {
    mat4 local_model;
    mat4 model;
    float normal_mapped;
};

#endif
