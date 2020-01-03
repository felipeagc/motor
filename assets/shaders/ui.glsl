blending: true
depth_test: false
depth_write: false
depth_bias: false
cull_mode: "back"
front_face: "clockwise"

vertex: [[
    layout(location = 0) in vec2 pos;
    layout(location = 1) in vec2 tex_coord;
    layout(location = 2) in vec3 color;

    layout (set = 0, binding = 0) uniform Transform {
        mat4 proj;
    };

    layout(location = 0) out vec2 tex_coord0;
    layout(location = 1) out vec3 color0;

    void main() {
        tex_coord0 = tex_coord;
        tex_coord0.y = tex_coord0.y;
        color0 = color;

        gl_Position = proj * vec4(pos, 0.0, 1.0);
    }
]]

fragment: [[
    layout(location = 0) in vec2 tex_coord;
    layout(location = 1) in vec3 in_color;

    layout (set = 0, binding = 1) uniform sampler2D bitmap;

    layout(location = 0) out vec4 out_color;

    void main() {
        out_color = vec4(in_color, texture(bitmap, tex_coord).r);
    }
]]
