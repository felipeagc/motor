depth_test = true
depth_write = true
cull_mode = "front"
front_face = "clockwise"

vertex = @{
    #include "common.glsl"

    layout (location = 0) in vec3 pos;
    layout (location = 1) in vec3 normal;
    layout (location = 2) in vec2 tex_coords;
    layout (location = 3) in vec4 tangent;

    layout (set = 0, binding = 0) uniform CameraUniform {
        Camera cam;
    };

    layout (set = 1, binding = 0) uniform ModelUniform {
        Model model;
    };

    void main() {
        mat4 model0 = model.model * model.local_model;
        vec4 loc_pos =  model0 * vec4(pos, 1.0);
        vec3 world_pos = loc_pos.xyz / loc_pos.w;
        gl_Position = cam.proj * cam.view * vec4(world_pos, 1.0f);
    }
}@
