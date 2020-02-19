#pragma motor vertex_entry vertex
#pragma motor pixel_entry pixel
#pragma motor blending false
#pragma motor depth_test false
#pragma motor depth_write false
#pragma motor cull_mode back
#pragma motor front_face clockwise

#include "common.hlsl"
#include "pbr_common.hlsl"

[[vk::binding(0, 0)]] cbuffer gizmo
{
    Camera cam;
    float4x4 model;
    float4 color;
};

struct VsInput
{
    float3 pos : POSITION;
};

struct VsOutput
{
    float4 sv_pos : SV_Position;
};

void vertex(in VsInput vs_in, out VsOutput vs_out)
{
    float4 loc_pos = mul(model, float4(vs_in.pos, 1.0));
    float3 world_pos = loc_pos.xyz / loc_pos.w;
    vs_out.sv_pos = mul(mul(cam.proj, cam.view), float4(world_pos, 1.0f));
}

float4 pixel(VsOutput fs_in) : SV_Target
{
    return srgb_to_linear(color);
}
