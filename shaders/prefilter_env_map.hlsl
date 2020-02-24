#pragma motor vertex_entry vertex
#pragma motor pixel_entry pixel
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

// Based omn
// http://byteblacksmith.com/improvements-to-the-canonical-one-liner-glsl-rand-for-opengl-es-2-0/
float random(float2 co)
{
    float a = 12.9898;
    float b = 78.233;
    float c = 43758.5453;
    float dt = dot(co.xy, float2(a, b));
    float sn = fmod(dt, 3.14);
    return frac(sin(sn) * c);
}

float2 hammersley2d(uint i, uint N)
{
    // Radical inverse based on http://holger.dammertz.org/stuff/notes_HammersleyOnHemisphere.html
    uint bits = (i << 16u) | (i >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    float rdi = float(bits) * 2.3283064365386963e-10;
    return float2(float(i) / float(N), rdi);
}

// Based on
// http://blog.selfshadow.com/publications/s2013-shading-course/karis/s2013_pbs_epic_slides.pdf
float3 importance_sample_ggx(float2 Xi, float roughness, float3 normal)
{
    // Maps a 2D point to a hemisphere with spread based on roughness
    float alpha = roughness * roughness;
    float phi = 2.0 * PI * Xi.x + random(normal.xz) * 0.1;
    float cos_theta = sqrt((1.0 - Xi.y) / (1.0 + (alpha * alpha - 1.0) * Xi.y));
    float sin_theta = sqrt(1.0 - cos_theta * cos_theta);
    float3 H = float3(sin_theta * cos(phi), sin_theta * sin(phi), cos_theta);

    // Tangent space
    float3 up = abs(normal.z) < 0.999 ? float3(0.0, 0.0, 1.0) : float3(1.0, 0.0, 0.0);
    float3 tangent_x = normalize(cross(up, normal));
    float3 tangent_y = normalize(cross(normal, tangent_x));

    // Convert to world Space
    return normalize(tangent_x * H.x + tangent_y * H.y + normal * H.z);
}

// Normal Distribution function
float d_ggx(float NdotH, float roughness)
{
    float alpha = roughness * roughness;
    float alpha2 = alpha * alpha;
    float denom = NdotH * NdotH * (alpha2 - 1.0) + 1.0;
    return (alpha2) / (PI * denom * denom);
}

float3 prefilter_env_map(float3 R, float roughness)
{
    const uint num_samples = 512;

    float3 N = R;
    float3 V = R;
    float3 color = float3(0.0);
    float total_weight = 0.0;
    float width, height, mip_levels;
    skybox.GetDimensions(0, width, height, mip_levels);
    float env_map_dim = width;
    for (uint i = 0u; i < num_samples; i++)
    {
        float2 Xi = hammersley2d(i, num_samples);
        float3 H = importance_sample_ggx(Xi, roughness, N);
        float3 L = 2.0 * dot(V, H) * H - V;
        float NdotL = clamp(dot(N, L), 0.0, 1.0);
        if (NdotL > 0.0)
        {
            // Filtering based on
            // https://placeholderart.wordpress.com/2015/07/28/implementation-notes-runtime-environment-map-filtering-for-image-based-lighting/

            float NdotH = clamp(dot(N, H), 0.0, 1.0);
            float VdotH = clamp(dot(V, H), 0.0, 1.0);

            // Probability Distribution Function
            float pdf = d_ggx(NdotH, roughness) * NdotH / (4.0 * VdotH) + 0.0001;
            // Slid angle of current smple
            float omega_s = 1.0 / (float(num_samples) * pdf);
            // Solid angle of 1 pixel across all cube faces
            float omega_p = 4.0 * PI / (6.0 * env_map_dim * env_map_dim);
            // Biased (+1.0) mip level for better result
            float mip_level =
                roughness == 0.0 ? 0.0 : max(0.5 * log2(omega_s / omega_p) + 1.0, 0.0f);
            color += skybox.SampleLevel(cube_sampler, L, mip_level).rgb * NdotL;
            total_weight += NdotL;
        }
    }
    return (color / total_weight);
}

float4 pixel(VsOutput fs_in) : SV_Target
{
    float3 N = normalize(fs_in.uvw);
    return float4(prefilter_env_map(N, roughness), 1.0);
}
