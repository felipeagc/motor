#pragma motor type graphics
#pragma motor blending true
#pragma motor depth_test true
#pragma motor depth_write true
#pragma motor depth_bias false
#pragma motor cull_mode back
#pragma motor front_face clockwise

[[vk::binding(0, 0)]] cbuffer transform
{
    float4x4 proj;
};

[[vk::binding(1, 0)]] SamplerState bitmap_sampler;
[[vk::binding(2, 0)]] Texture2D<float4> bitmap;

struct VsOutput
{
    float4 sv_pos : SV_Position;
    float2 uv : TEXCOORD;
    float3 color : COLOR;
};

VsOutput vertex(float2 pos, float2 uv, float3 color)
{
    VsOutput vs_out;
    vs_out.uv = uv;
    vs_out.color = color;
    vs_out.sv_pos = mul(proj, float4(pos, 0.0, 1.0));
    return vs_out;
}

float4 pixel(VsOutput vs_out) : SV_Target
{
    float4 sampled_color = bitmap.Sample(bitmap_sampler, vs_out.uv);
    return float4(vs_out.color * sampled_color.rgb, sampled_color.a);
}
