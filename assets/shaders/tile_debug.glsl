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
    #include "common.glsl"

    layout(location = 0) in vec2 tex_coord;

    layout (set = 0, binding = 0) readonly buffer VisibleLightsBuffer {
        uint num_tiles_x;
        int indices[];
    } visible_lights_buffer;

    layout(location = 0) out vec4 out_color;

    void main()
    {
        ivec2 location = ivec2(gl_FragCoord.xy);
        ivec2 tile_id = location / ivec2(TILE_SIZE, TILE_SIZE);
        uint index = tile_id.y * visible_lights_buffer.num_tiles_x + tile_id.x;
        uint offset = index * MAX_POINT_LIGHTS;

        float count = 0;
        for (uint i = 0; i < MAX_POINT_LIGHTS && visible_lights_buffer.indices[offset+i] != -1; i++) {
            count += 1.0f;
        }

        count /= float(MAX_POINT_LIGHTS);

        out_color = vec4(0.0f, 0.0f, count, 1.0f);
    }
}@
