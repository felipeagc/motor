blending = false
depth_test = false
depth_write = false
depth_bias = false
cull_mode = "front"
front_face = "counter_clockwise"

vertex = @{
    layout(location = 0) out vec2 tex_coord0;

    void main()
    {
        tex_coord0 = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
        gl_Position = vec4(tex_coord0 * 2.0f + -1.0f, 0.0f, 1.0f);
    }
}@

fragment = @{
    #include "pbr_common.glsl"

    layout(location = 0) in vec2 tex_coord;

    layout (set = 0, binding = 0) uniform sampler2D in_color;
    layout (set = 0, binding = 1) uniform sampler2D in_emissive;

    layout(location = 0) out vec4 out_color;

    void main()
    {
        float weight[5];
        weight[0] = 0.227027;
        weight[1] = 0.1945946;
        weight[2] = 0.1216216;
        weight[3] = 0.054054;
        weight[4] = 0.016216;

        const float blur_strength = 1.0f;
        const float blur_scale = 1.0f;

        vec2 tex_offset = 1.0 / textureSize(in_emissive, 0) * blur_scale; // gets size of single texel
        vec3 bloom_color = texture(in_emissive, tex_coord).rgb * weight[0]; // current fragment's contribution
        for (int i = 1; i < 5; ++i)
        {
            vec2 voff = vec2(0.0, tex_offset.y * i);
            bloom_color += texture(in_emissive, tex_coord + voff).rgb * weight[i] * blur_strength;
            bloom_color += texture(in_emissive, tex_coord - voff).rgb * weight[i] * blur_strength;

            vec2 hoff = vec2(tex_offset.y * i, 0.0);
            bloom_color += texture(in_emissive, tex_coord + hoff).rgb * weight[i] * blur_strength;
            bloom_color += texture(in_emissive, tex_coord - hoff).rgb * weight[i] * blur_strength;
        }

        vec3 hdr_color = texture(in_color, tex_coord).rgb;      
        // hdr_color += bloom_color; // additive blending
        // tone mapping
        // vec4 result = tonemap(vec4(hdr_color, 1.0), exposure);
        // vec3 result = vec3(1.0) - exp(-hdr_color * exposure);
        // // also gamma correct while we're at it       
        // result = pow(result, vec3(1.0 / gamma));
        out_color = vec4(hdr_color, 1.0);

        // out_color = vec4(result, 1.0);
        // out_color = vec4(texture(in_color, tex_coord).rgb, 1.0);
        // out_color = vec4(texture(in_color, tex_coord).rgb + result, 1.0);
    }
}@
