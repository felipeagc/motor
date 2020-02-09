#pragma motor type graphics
#pragma motor blending false
#pragma motor depth_test false
#pragma motor depth_write false
#pragma motor cull_mode none
#pragma motor front_face clockwise

#define PI 3.1415926536

[[vk::binding(0, 0)]] cbuffer radiance
{
    float4x4 mvp;
    float roughness;
};

[[vk::binding(1, 0)]] SamplerState cube_sampler;
[[vk::binding(2, 0)]] TextureCube<float4> skybox;

struct VsOutput
{
    float4 pos : SV_Position;
    float3 uvw : TEXCOORD;
};

void vertex(out VsOutput vs_out, in float3 pos)
{
    vs_out.uvw = pos;
    vs_out.uvw.y *= -1.0;
    vs_out.pos = mul(mvp, float4(pos, 1.0));
}

float4 pixel(VsOutput fs_in) : SV_Target
{
    float3 N = normalize(fs_in.uvw);
    float3 up = float3(0.0, 1.0, 0.0);
    float3 right = normalize(cross(up, N));
    up = cross(N, right);

    const float TWO_PI = PI * 2.0;
    const float HALF_PI = PI * 0.5;

    const float delta_phi = (2.0f * PI) / 180.0f;
    const float delta_theta = (0.5f * PI) / 64.0f;

    float3 color = float3(0.0);
    uint sample_count = 0u;
    for (float phi = 0.0; phi < TWO_PI; phi += delta_phi)
    {
        for (float theta = 0.0; theta < HALF_PI; theta += delta_theta)
        {
            float3 temp_vec = cos(phi) * right + sin(phi) * up;
            float3 sample_vector = cos(theta) * N + sin(theta) * temp_vec;
            color += skybox.Sample(cube_sampler, sample_vector).rgb * cos(theta) * sin(theta);
            sample_count++;
        }
    }

    return float4(PI * color / float(sample_count), 1.0);
}
