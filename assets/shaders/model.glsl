cull_mode: "back"
front_face: "counter_clockwise"

vertex: [[
    layout (location = 0) in vec3 pos;
    layout (location = 1) in vec3 normal;
    layout (location = 2) in vec2 tex_coords;

    layout (set = 0, binding = 0) uniform LocalModel {
        mat4 local_model;
    };

    layout (location = 0) out vec3 normal0;
    layout (location = 1) out vec2 tex_coords0;

    void main() {
        normal0 = normal;
        tex_coords0 = tex_coords;
        tex_coords0.y = 1.0 - tex_coords0.y;

        gl_Position = local_model * vec4(pos.xyz, 1.0f);
    }
]]

fragment: [[
    layout (location = 0) in vec3 normal;
    layout (location = 1) in vec2 tex_coords;

    layout (location = 0) out vec4 out_color;
    
    void main() {
        out_color = vec4(1.0f);
    }
]]
