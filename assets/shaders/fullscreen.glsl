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
    layout(location = 0) in vec2 tex_coord;

    layout (set = 0, binding = 0) uniform sampler2D albedo;

    layout(location = 0) out vec4 out_color;

    void main()
    {
        out_color = vec4(texture(albedo, tex_coord).rgb, 1.0);
    }
}@
