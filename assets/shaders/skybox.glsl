blending: false
depth_test: false
depth_write: false
cull_mode: "front"
front_face: "clockwise"

common: [[
    #define MAX_LIGHTS 20

    struct Camera {
        mat4 view;
        mat4 proj;
        vec4 pos;
    };

    struct PointLight {
        vec4 pos;
        vec4 color;
    };

    struct Environment {
        vec3 sun_direction;
        float exposure;

        vec3 sun_color;
        float sun_intensity;

        mat4 light_space_matrix;

        float radiance_mip_levels;
        uint light_count;

        float dummy1;
        float dummy2;

        PointLight lights[MAX_LIGHTS];
    };
]]

vertex: [[
    layout(location = 0) in vec3 pos;

    layout(set = 0, binding = 0) uniform CameraUniform {
        Camera cam;
    };

    layout(location = 0) out vec3 tex_coords0;

    void main() {
        tex_coords0 = pos;
        tex_coords0.y *= -1.0f;

        mat4 view = cam.view;
        view[3][0] = 0.0;
        view[3][1] = 0.0;
        view[3][2] = 0.0;

        vec4 position = cam.proj * view * vec4(pos * vec3(100.0), 1.0);
        gl_Position = position.xyww;
    }
]]

fragment: [[
    layout (location = 0) in vec3 tex_coords;

    layout (set = 1, binding = 0) uniform EnvironmentUniform {
        Environment environment;
    };
    layout (set = 1, binding = 1) uniform samplerCube env_map;

    layout (location = 0) out vec4 out_color;

    void main() {
        out_color = pow(vec4(texture(env_map, tex_coords).rgb, 1.0), vec4(2.2));
        out_color = vec4(1.0) - exp(-out_color * environment.exposure);
        out_color = pow(out_color, vec4(1.0/2.2));
    }
]]
