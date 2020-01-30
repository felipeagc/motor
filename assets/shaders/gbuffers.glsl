blending = true
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

    layout (location = 0) out vec2 tex_coords0;
    layout (location = 1) out vec3 world_pos;
    layout (location = 2) out vec3 normal0;
    layout (location = 3) out mat3 TBN;

    void main() {
        tex_coords0 = tex_coords;

        mat4 model0 = model.model * model.local_model;

        if (model.normal_mapped == 1.0f) {
            vec3 T = normalize(vec3(model0 * vec4(tangent.xyz, 0.0f)));
            vec3 N = normalize(vec3(model0 * vec4(normal.xyz, 0.0f)));
            T = normalize(T - dot(T, N) * N); // re-orthogonalize
            vec3 B = tangent.w * cross(N, T);
            TBN = mat3(T, B, N);
        } else {
            normal0 = mat3(transpose(inverse(model0))) * normal;
        }

        vec4 loc_pos =  model0 * vec4(pos, 1.0);

        world_pos = loc_pos.xyz / loc_pos.w;

        gl_Position = cam.proj * cam.view * vec4(world_pos, 1.0f);
    }
}@

fragment = @{
    #include "common.glsl"
    #include "pbr_common.glsl"

    layout (location = 0) in vec2 tex_coords;
    layout (location = 1) in vec3 world_pos;
    layout (location = 2) in vec3 normal;
    layout (location = 3) in mat3 TBN;

    layout (set = 1, binding = 0) uniform ModelUniform {
        Model model;
    };

    layout (set = 2, binding = 0) uniform MaterialUniform {
        Material material;
    };
    layout (set = 2, binding = 1) uniform sampler2D albedo_texture;
    layout (set = 2, binding = 2) uniform sampler2D normal_texture;
    layout (set = 2, binding = 3) uniform sampler2D metallic_roughness_texture;
    layout (set = 2, binding = 4) uniform sampler2D occlusion_texture;
    layout (set = 2, binding = 5) uniform sampler2D emissive_texture;

    layout (location = 0) out vec4 out_albedo;
    layout (location = 1) out vec4 out_metallic_roughness;
    layout (location = 2) out vec4 out_occlusion;
    layout (location = 3) out vec4 out_emissive;
    layout (location = 4) out vec4 out_pos;
    layout (location = 5) out vec4 out_normal;
    
    void main() {
        vec3 N;
        if (model.normal_mapped == 1.0f) {
            N = texture(normal_texture, tex_coords).rgb;
            N = normalize(N * 2.0 - 1.0); // Remap from [0, 1] to [-1, 1]
            N = normalize(TBN * N);
        } else {
            N = normalize(normal);
        }

        out_albedo = texture(albedo_texture, tex_coords) * material.base_color;

        out_metallic_roughness.bg = texture(metallic_roughness_texture, tex_coords).bg;
        out_metallic_roughness.b *= material.metallic;
        out_metallic_roughness.g *= material.roughness;
        out_metallic_roughness.ra = vec2(0.0, 1.0);

        out_occlusion.r = texture(occlusion_texture, tex_coords).r;
        out_occlusion.gba = vec3(0, 0, 1);

        out_emissive.rgb = texture(emissive_texture, tex_coords).rgb * material.emissive.rgb;
        out_emissive.a = 1.0;

        out_pos.xyz = world_pos;
        out_pos.w = 1.0;

        out_normal.xyz = N;
        out_normal.w = 1.0;
    }
}@
