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

    const float GAMMA = 2.2f;

    vec4 srgb_to_linear(vec4 srgb_in) {
        /* vec3 lin_out = pow(srgb_in.xyz, vec3(GAMMA)); */

        vec3 b_less = step(vec3(0.04045),srgb_in.xyz);
        vec3 lin_out = mix(
                srgb_in.xyz/vec3(12.92),
                pow((srgb_in.xyz+vec3(0.055))/vec3(1.055),vec3(2.4)),
                b_less);
        return vec4(lin_out, srgb_in.w);
    }

    vec3 uncharted2_tonemap(vec3 color) {
        float A = 0.15;
        float B = 0.50;
        float C = 0.10;
        float D = 0.20;
        float E = 0.02;
        float F = 0.30;
        float W = 11.2;
        return ((color*(A*color+C*B)+D*E)/(color*(A*color+B)+D*F))-E/F;
    }

    vec4 tonemap(vec4 color, float exposure) {
        vec3 outcol = uncharted2_tonemap(color.rgb * exposure);
        outcol = outcol * (1.0f / uncharted2_tonemap(vec3(11.2f)));	
        return vec4(pow(outcol, vec3(1.0f / GAMMA)), color.a);
    }
]]

vertex: [[
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
