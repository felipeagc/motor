#pragma motor vertex_entry vertex
#pragma motor pixel_entry pixel
#pragma motor blending true
#pragma motor depth_test true
#pragma motor depth_write true
#pragma motor cull_mode front
#pragma motor front_face clockwise

#include "common.hlsl"
#include "pbr_common.hlsl"

[[vk::binding(0, 0)]] ConstantBuffer<Camera> cam;

[[vk::binding(0, 1)]] ConstantBuffer<Model> model;

[[vk::binding(0, 2)]] ConstantBuffer<Material> material;
[[vk::binding(1, 2)]] SamplerState texture_sampler;
[[vk::binding(2, 2)]] Texture2D<float4> albedo_texture;
[[vk::binding(3, 2)]] Texture2D<float4> normal_texture;
[[vk::binding(4, 2)]] Texture2D<float4> metallic_roughness_texture;
[[vk::binding(5, 2)]] Texture2D<float4> occlusion_texture;
[[vk::binding(6, 2)]] Texture2D<float4> emissive_texture;

[[vk::binding(0, 3)]] ConstantBuffer<Environment> env;
[[vk::binding(1, 3)]] SamplerState cube_sampler;
[[vk::binding(2, 3)]] SamplerState radiance_sampler;
[[vk::binding(3, 3)]] TextureCube<float4> irradiance_map;
[[vk::binding(4, 3)]] TextureCube<float4> radiance_map;
[[vk::binding(5, 3)]] Texture2D<float4> brdf_lut;

struct VsInput
{
    float3 pos : POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD;
    float4 tangent : TANGENT;
};

struct VsOutput
{
    float4 sv_pos : SV_Position;
    float2 uv : TEXCOORD;
    float3 world_pos : POSITION;
    float3 normal : NORMAL;
    float3x3 TBN : TBN_MATRIX;
};

void vertex(in VsInput vs_in, out VsOutput vs_out)
{
    vs_out.uv = vs_in.uv;

    if (material.normal_mapped == 1.0f)
    {
        float3 T = normalize(float3(mul(model.mat, float4(vs_in.tangent.xyz, 0.0f))));
        float3 N = normalize(float3(mul(model.mat, float4(vs_in.normal.xyz, 0.0f))));
        T = normalize(T - dot(T, N) * N); // re-orthogonalize
        float3 B = vs_in.tangent.w * cross(N, T);
        vs_out.TBN = transpose(float3x3(T, B, N));
    }
    else
    {
        vs_out.normal = normalize(mul(float3x3(model.mat), vs_in.normal));
    }

    float4 loc_pos = mul(model.mat, float4(vs_in.pos, 1.0));

    vs_out.world_pos = loc_pos.xyz / loc_pos.w;

    vs_out.sv_pos = mul(mul(cam.proj, cam.view), float4(vs_out.world_pos, 1.0f));
}

float3 get_ibl_contribution(PBRInfo pbr_inputs)
{
    uint width, height, radiance_mip_levels;
    radiance_map.GetDimensions(0, width, height, radiance_mip_levels);

    float lod = (pbr_inputs.perceptual_roughness * float(radiance_mip_levels));
    // retrieve a scale and bias to F0. See [1], Figure 3
    float3 brdf =
        brdf_lut
            .Sample(
                texture_sampler, float2(pbr_inputs.NdotV, 1.0 - pbr_inputs.perceptual_roughness))
            .rgb;
    float3 diffuse_light =
        srgb_to_linear(tonemap(irradiance_map.Sample(cube_sampler, pbr_inputs.N), env.exposure))
            .rgb;

    float3 specular_light =
        srgb_to_linear(
            tonemap(radiance_map.SampleLevel(radiance_sampler, pbr_inputs.R, lod), env.exposure))
            .rgb;

    float3 diffuse = diffuse_light * pbr_inputs.diffuse_color;
    float3 specular = specular_light * (pbr_inputs.specular_color * brdf.x + brdf.y);

    return diffuse + specular;
}

float3 diffuse(PBRInfo pbr_inputs)
{
    return pbr_inputs.diffuse_color / PI;
}

float3 specular_reflection(PBRInfo pbr_inputs, LightInfo light_info)
{
    return pbr_inputs.reflectance0 + (pbr_inputs.reflectance90 - pbr_inputs.reflectance0) *
                                         pow(clamp(1.0 - light_info.VdotH, 0.0, 1.0), 5.0);
}

float geometric_occlusion(PBRInfo pbr_inputs, LightInfo light_info)
{
    float NdotL = light_info.NdotL;
    float NdotV = pbr_inputs.NdotV;
    float r = pbr_inputs.alpha_roughness;

    float attenuationL = 2.0 * NdotL / (NdotL + sqrt(r * r + (1.0 - r * r) * (NdotL * NdotL)));
    float attenuationV = 2.0 * NdotV / (NdotV + sqrt(r * r + (1.0 - r * r) * (NdotV * NdotV)));
    return attenuationL * attenuationV;
}

float microfacet_distribution(PBRInfo pbr_inputs, LightInfo light_info)
{
    float roughness_sq = pbr_inputs.alpha_roughness * pbr_inputs.alpha_roughness;
    float f = (light_info.NdotH * roughness_sq - light_info.NdotH) * light_info.NdotH + 1.0;
    return roughness_sq / (PI * f * f);
}

float4 pixel(VsOutput fs_in) : SV_Target
{
    float4 albedo =
        srgb_to_linear(albedo_texture.Sample(texture_sampler, fs_in.uv)) * material.base_color;
    float4 metallic_roughness = metallic_roughness_texture.Sample(texture_sampler, fs_in.uv);
    float occlusion = occlusion_texture.Sample(texture_sampler, fs_in.uv).r;
    float3 emissive = srgb_to_linear(emissive_texture.Sample(texture_sampler, fs_in.uv)).rgb *
                      material.emissive.rgb;

    // Calculate PBR
    PBRInfo pbr_inputs;

    pbr_inputs.metallic = material.metallic * metallic_roughness.b;
    pbr_inputs.perceptual_roughness = material.roughness * metallic_roughness.g;

    float3 f0 = float3(0.04);
    pbr_inputs.diffuse_color = albedo.rgb * (float3(1.0) - f0);
    pbr_inputs.diffuse_color *= 1.0 - pbr_inputs.metallic;

    pbr_inputs.alpha_roughness = pbr_inputs.perceptual_roughness * pbr_inputs.perceptual_roughness;

    pbr_inputs.specular_color = lerp(f0, albedo.rgb, pbr_inputs.metallic);

    pbr_inputs.reflectance0 = pbr_inputs.specular_color.rgb;

    float3 reflectance = max(
        max(pbr_inputs.specular_color.r, pbr_inputs.specular_color.g), pbr_inputs.specular_color.b);
    pbr_inputs.reflectance90 = clamp(reflectance * 25.0, 0.0, 1.0);

    if (material.normal_mapped == 1.0f)
    {
        pbr_inputs.N = normal_texture.Sample(texture_sampler, fs_in.uv).rgb;
        pbr_inputs.N = normalize(pbr_inputs.N * 2.0 - 1.0); // Remap from [0, 1] to [-1, 1]
        pbr_inputs.N = normalize(mul(fs_in.TBN, pbr_inputs.N));
    }
    else
    {
        pbr_inputs.N = normalize(fs_in.normal);
    }

    pbr_inputs.V = normalize(cam.pos.xyz - fs_in.world_pos);
    pbr_inputs.R = -normalize(reflect(pbr_inputs.V, pbr_inputs.N)); // TODO: may need to flip R.y
    pbr_inputs.NdotV = clamp(abs(dot(pbr_inputs.N, pbr_inputs.V)), 0.001, 1.0);

    float3 Lo = float3(0.0);

    // Directional light (sun)
    {
        float3 L = normalize(-env.sun_direction);
        float3 H = normalize(pbr_inputs.V + L);
        float3 light_color = env.sun_color * env.sun_intensity;

        LightInfo light_info;
        light_info.NdotL = clamp(dot(pbr_inputs.N, L), 0.001, 1.0);
        light_info.NdotH = clamp(dot(pbr_inputs.N, H), 0.0, 1.0);
        light_info.VdotH = clamp(dot(pbr_inputs.V, H), 0.0, 1.0);

        float3 F = specular_reflection(pbr_inputs, light_info);
        float G = geometric_occlusion(pbr_inputs, light_info);
        float D = microfacet_distribution(pbr_inputs, light_info);

        float3 diffuse_contrib = (1.0 - F) * diffuse(pbr_inputs);
        float3 spec_contrib = F * G * D / (4.0 * light_info.NdotL * pbr_inputs.NdotV);

        float3 color = light_info.NdotL * light_color * (diffuse_contrib + spec_contrib);
        Lo += color;
    }

    //
    // Point lights
    //

    for (uint i = 0; i < env.light_count; i++)
    {
        PointLight light = env.point_lights[i];

        // Calculate per-light radiance
        float3 light_pos = light.pos.xyz;
        float3 L = normalize(light_pos - fs_in.world_pos);
        float3 H = normalize(pbr_inputs.V + L);
        float distance = length(light_pos - fs_in.world_pos);
        float attenuation = 1.0 / (distance * distance);
        float3 light_color = light.color.rgb * attenuation;

        LightInfo light_info;
        light_info.NdotL = clamp(dot(pbr_inputs.N, L), 0.001, 1.0);
        light_info.NdotH = clamp(dot(pbr_inputs.N, H), 0.0, 1.0);
        light_info.VdotH = clamp(dot(pbr_inputs.V, H), 0.0, 1.0);

        float3 F = specular_reflection(pbr_inputs, light_info);
        float G = geometric_occlusion(pbr_inputs, light_info);
        float D = microfacet_distribution(pbr_inputs, light_info);

        float3 diffuse_contrib = (1.0 - F) * diffuse(pbr_inputs);
        float3 spec_contrib = F * G * D / (4.0 * light_info.NdotL * pbr_inputs.NdotV);

        float3 color = light_info.NdotL * light_color * (diffuse_contrib + spec_contrib);
        Lo += color;
    }

    Lo += get_ibl_contribution(pbr_inputs);

    Lo *= occlusion;
    Lo += emissive;

    return float4(Lo, albedo.a);
}
