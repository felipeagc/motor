#pragma motor vertex_entry vertex
#pragma motor pixel_entry pixel
#pragma motor blending true
#pragma motor depth_test true
#pragma motor depth_write true
#pragma motor cull_mode front
#pragma motor front_face clockwise

#include "common.hlsl"
#include "pbr_common.hlsl"

[[vk::binding(0, 0)]] cbuffer camera
{
    Camera cam;
};

[[vk::binding(0, 1)]] cbuffer model
{
    Model model;
};

[[vk::binding(0, 2)]] cbuffer picker
{
    uint entity_id;
};

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
};

void vertex(in VsInput vs_in, out VsOutput vs_out)
{
    float4x4 model0 = mul(model.model, model.local_model);
    float4 loc_pos = mul(model0, float4(vs_in.pos, 1.0));
    float3 world_pos = loc_pos.xyz / loc_pos.w;
    vs_out.sv_pos = mul(mul(cam.proj, cam.view), float4(world_pos, 1.0f));
}

uint pixel(VsOutput fs_in) : SV_Target
{
    return entity_id + 1;
}
