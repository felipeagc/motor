#pragma motor type graphics
#pragma motor blending false
#pragma motor depth_test false
#pragma motor depth_write false
#pragma motor cull_mode front
#pragma motor front_face clockwise

#include "common.hlsl"
#include "pbr_common.hlsl"

[[vk::binding(0, 0)]] cbuffer camera
{
    Camera cam;
};

[[vk::binding(0, 1)]] SamplerState cube_sampler;
[[vk::binding(1, 1)]] TextureCube<float4> cube_texture;
[[vk::binding(2, 1)]] cbuffer environment
{
    Environment env;
};

struct VsOutput
{
    float4 sv_pos : SV_Position;
    float3 uvw : TEXCOORD;
};

VsOutput vertex(float3 pos)
{
    VsOutput vs_out;

    vs_out.uvw = pos;

    float4x4 view = cam.view;
    view[0][3] = 0.0;
    view[1][3] = 0.0;
    view[2][3] = 0.0;

    vs_out.sv_pos = mul(mul(cam.proj, view), float4(pos * float3(100.0), 1.0));
    vs_out.sv_pos = vs_out.sv_pos.xyww;
    return vs_out;
}

float4 pixel(VsOutput vs_out) : SV_Target
{
    return tonemap(
        srgb_to_linear(cube_texture.SampleLevel(cube_sampler, vs_out.uvw, 1.5)), env.exposure);
}
