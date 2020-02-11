#pragma motor vertex_entry vertex
#pragma motor pixel_entry pixel
#pragma motor blending true
#pragma motor depth_test false
#pragma motor depth_write false
#pragma motor depth_bias false
#pragma motor cull_mode none
#pragma motor front_face clockwise

#include "pbr_common.hlsl"

[[vk::binding(0, 0)]] cbuffer transform
{
    float4x4 proj;
};

[[vk::binding(1, 0)]] SamplerState bitmap_sampler;
[[vk::binding(2, 0)]] Texture2D<float4> bitmap;

struct VsInput
{
    float2 pos : POSITION;
    float2 uv : TEXCOORD;
    float4 color : COLOR;
};

struct VsOutput
{
    float4 sv_pos : SV_Position;
    float2 uv : TEXCOORD;
    float4 color : COLOR;
};

void vertex(out VsOutput vs_out, in VsInput vs_in)
{
    vs_out.uv = vs_in.uv;
    vs_out.color = vs_in.color;
    vs_out.sv_pos = mul(proj, float4(vs_in.pos, 0.0, 1.0));
}

float4 pixel(in VsOutput fs_in) : SV_Target
{
    return srgb_to_linear(bitmap.Sample(bitmap_sampler, fs_in.uv) * fs_in.color);
}
