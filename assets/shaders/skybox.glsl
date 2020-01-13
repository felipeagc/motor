blending: false
depth_test: false
depth_write: false
cull_mode: "front"
front_face: "clockwise"

vertex: [[
    #include "common.glsl"

    layout(location = 0) in vec3 pos;

    layout(set = 0, binding = 0) uniform CameraUniform {
        Camera cam;
    };

    layout(location = 0) out vec3 tex_coords0;

    void main() {
        tex_coords0 = pos;
        /* tex_coords0.y *= -1.0f; */

        mat4 view = cam.view;
        view[3][0] = 0.0;
        view[3][1] = 0.0;
        view[3][2] = 0.0;

        vec4 position = cam.proj * view * vec4(pos * vec3(100.0), 1.0);
        gl_Position = position.xyww;
    }
]]

fragment: [[
    #include "common.glsl"
    #include "pbr_common.glsl"

    layout (location = 0) in vec3 tex_coords;

    layout (set = 1, binding = 0) uniform EnvironmentUniform {
        Environment environment;
    };
    layout (set = 1, binding = 1) uniform samplerCube env_map;

    layout (location = 0) out vec4 out_color;

    void main() {
        out_color.rgb = srgb_to_linear(tonemap(textureLod(env_map, tex_coords, 1.5), environment.exposure)).rgb;
        out_color.a = 1.0f;
    }
]]
