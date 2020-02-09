#ifndef PBR_COMMON_HLSL
#define PBR_COMMON_HLSL

#ifndef PI
#define PI 3.14159265359
#endif

#ifndef GAMMA
#define GAMMA 2.2f
#endif

struct PBRInfo
{
    float3 V;
    float3 N;
    float3 R;
    float NdotV; // cos angle between normal and view direction
    float perceptual_roughness;
    float metallic;
    float3 reflectance0;   // full reflectance color (normal incidence angle)
    float3 reflectance90;  // reflectance color at grazing angle
    float alpha_roughness; // roughness mapped to a more linear change in the roughness
    float3 diffuse_color;  // color contribution from diffuse lighting
    float3 specular_color; // color contribution from specular lighting
};

struct LightInfo
{
    float NdotL; // cos angle between normal and light direction
    float NdotH; // cos angle between normal and half vector
    float LdotH; // cos angle between light direction and half vector
    float VdotH; // cos angle between view direction and half vector
};

float4 srgb_to_linear(float4 srgb_in)
{
    float3 b_less = step(float3(0.04045), srgb_in.xyz);
    float3 lin_out = lerp(
        srgb_in.xyz / float3(12.92),
        pow((srgb_in.xyz + float3(0.055)) / float3(1.055), float3(2.4)),
        b_less);
    return float4(lin_out, srgb_in.a);
}

float3 uncharted2_tonemap(float3 color)
{
    float A = 0.15;
    float B = 0.50;
    float C = 0.10;
    float D = 0.20;
    float E = 0.02;
    float F = 0.30;
    float W = 11.2;
    return ((color * (A * color + C * B) + D * E) / (color * (A * color + B) + D * F)) - E / F;
}

float4 tonemap(float4 color, float exposure)
{
    float3 outcol = uncharted2_tonemap(color.rgb * exposure);
    outcol = outcol * (1.0f / uncharted2_tonemap(float3(11.2f)));
    return float4(pow(outcol, float3(1.0f / GAMMA)), color.a);
}

#endif
