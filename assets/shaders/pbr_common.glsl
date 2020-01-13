#ifndef PBR_COMMON_GLSL
#define PBR_COMMON_GLSL

#ifndef PI
#define PI 3.14159265359
#endif

const float GAMMA = 2.2f;

vec4 srgb_to_linear(vec4 srgb_in) {
    vec3 lin_out = pow(srgb_in.xyz, vec3(GAMMA));
    return vec4(lin_out, srgb_in.w);
}

vec3 uncharted2_tonemap(vec3 color) {
    float A = 0.15;
    float B = 0.50;
    float C = 0.10;
    float D = 0.20;
    float E = 0.02;
    float F = 0.30;
    float W = 11.2;
    return ((color*(A*color+C*B)+D*E)/(color*(A*color+B)+D*F))-E/F;
}

vec4 tonemap(vec4 color, float exposure) {
    vec3 outcol = uncharted2_tonemap(color.rgb * exposure);
    outcol = outcol * (1.0f / uncharted2_tonemap(vec3(11.2f)));	
    return vec4(pow(outcol, vec3(1.0f / GAMMA)), color.a);
}

vec3 fresnel_schlick(float cos_theta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cos_theta, 0.0, 1.0), 5.0);
}

vec3 fresnel_schlick_roughness(float cos_theta, vec3 F0, float roughness) {
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(1.0 - cos_theta, 5.0);
}

float distribution_ggx(vec3 N, vec3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = clamp(dot(N, H), 0.0, 1.0);
    float NdotH2 = NdotH * NdotH;

    float nom = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / denom;
}

float geometry_schlick_smith_ggx(float NdotL, float NdotV, float roughness) {
    float r = roughness + 1.0;
    float k = (r*r) / 8.0;
    float GL = NdotL / (NdotL * (1.0 - k) + k);
    float GV = NdotV / (NdotV * (1.0 - k) + k);
    return GL * GV;
}

#endif
