#pragma motor vertex_entry vertex
#pragma motor pixel_entry pixel

#define NUM_SAMPLES 1024u
#define PI 3.1415926536

struct VsOutput
{
    float4 sv_pos : SV_Position;
    float2 uv : TEXCOORD;
};

void vertex(out VsOutput vs_out, in uint id : SV_VertexID)
{
    vs_out.uv = float2((id << 1) & 2, id & 2);
    vs_out.sv_pos = float4(vs_out.uv * 2.0f + -1.0f, 0.0f, 1.0f);
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

// Geometric Shadowing function
float g_schlicksmith_ggx(float NdotL, float NdotV, float roughness)
{
    float k = (roughness * roughness) / 2.0;
    float GL = NdotL / (NdotL * (1.0 - k) + k);
    float GV = NdotV / (NdotV * (1.0 - k) + k);
    return GL * GV;
}

float2 brdf(float NoV, float roughness)
{
    // Normal always points along z-axis for the 2D lookup
    const float3 N = float3(0.0, 0.0, 1.0);
    float3 V = float3(sqrt(1.0 - NoV * NoV), 0.0, NoV);

    float2 lut = float2(0.0);
    for (uint i = 0u; i < NUM_SAMPLES; i++)
    {
        float2 xi = hammersley2d(i, NUM_SAMPLES);
        float3 H = importance_sample_ggx(xi, roughness, N);
        float3 L = 2.0 * dot(V, H) * H - V;

        float NdotL = max(dot(N, L), 0.0);
        float NdotV = max(dot(N, V), 0.0);
        float VdotH = max(dot(V, H), 0.0);
        float NdotH = max(dot(H, N), 0.0);

        if (NdotL > 0.0)
        {
            float g = g_schlicksmith_ggx(NdotL, NdotV, roughness);
            float g_vis = (g * VdotH) / (NdotH * NdotV);
            float fc = pow(1.0 - VdotH, 5.0);
            lut += float2((1.0 - fc) * g_vis, fc * g_vis);
        }
    }
    return lut / float(NUM_SAMPLES);
}

float4 pixel(VsOutput fs_in) : SV_Target
{
    return float4(brdf(fs_in.uv.s, 1.0 - fs_in.uv.t), 0.0, 1.0);
}
