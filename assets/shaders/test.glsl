vertex: [[
    layout (location = 0) in vec3 pos;
    layout (location = 1) in vec3 normal;
    layout (location = 2) in vec2 tex_coords;

    layout (location = 0) out vec3 normal0;
    layout (location = 1) out vec2 tex_coords0;

    void main() {
        normal0 = normal;
        tex_coords0 = tex_coords;

        gl_Position = vec4(pos, 1.0f);
    }
]]

fragment: [[
    layout (location = 0) in vec3 normal;
    layout (location = 1) in vec2 tex_coords;

    layout (set = 0, binding = 0) uniform sampler2D albedo;
    layout (set = 0, binding = 1) uniform MyUniform {
        vec3 color;
    };

    layout (location = 0) out vec4 out_color;

    
    void main() {
        out_color = texture(albedo, tex_coords) * vec4(color, 1.0f);
    }
]]
