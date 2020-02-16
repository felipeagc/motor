#pragma motor vertex_entry vertex
#pragma motor pixel_entry pixel
#pragma motor blending false
#pragma motor depth_test false
#pragma motor depth_write false
#pragma motor depth_bias false
#pragma motor cull_mode front
#pragma motor front_face counter_clockwise

[[vk::binding(0, 0)]] SamplerState image_sampler;
[[vk::binding(1, 0)]] Texture2D<float4> image;

struct VsOutput
{
    float4 sv_pos : SV_Position;
    float2 uv : TEXCOORD;
};

VsOutput vertex(uint id : SV_VertexID)
{
    VsOutput vs_out;
    vs_out.uv = float2((id << 1) & 2, id & 2);
    vs_out.sv_pos = float4(vs_out.uv * 2.0f + -1.0f, 0.0f, 1.0f);
    return vs_out;
}

float4 pixel(VsOutput vs_out) : SV_Target
{
    return float4(image.Sample(image_sampler, vs_out.uv).rgb, 1.0);
}
