compute = @{
    #include "common.glsl"

    layout (set = 0, binding = 0) uniform sampler2D depth_sampler;

    layout (set = 0, binding = 1) uniform CameraUniform {
        Camera cam;
    };

    layout (set = 0, binding = 2) uniform EnvironmentUniform {
        Environment env;
    };

    layout (set = 0, binding = 3) writeonly buffer VisibleLightsBuffer {
        uint num_tiles_x;
        int indices[];
    } visible_lights_buffer;

    struct ViewFrustum
    {
        vec4 planes[6];
        vec3 points[8]; // 0-3 near 4-7 far
    };

    const vec2 ndc_upper_left = vec2(-1.0, -1.0);
    const float ndc_near_plane = 0.0;
    const float ndc_far_plane = 1.0;

    shared uint min_depth_uint;
    shared uint max_depth_uint;
    shared uint visible_light_count;
    shared int visible_light_indices[MAX_POINT_LIGHTS];
    shared mat4 proj_view;
    shared mat4 inv_proj_view;
    shared mat4 inv_proj;

    float linearize_depth(float depth) {
        return proj_view[3][2] / (depth + proj_view[2][2]);
    }

    vec3 create_plane_eqn(vec3 b, vec3 c) {
        // normalize(cross( b-a, c-a )), except we know "a" is the origin
        // also, typically there would be a fourth term of the plane equation, 
        // -(n dot a), except we know "a" is the origin
        return normalize(cross(b,c));;
    }

    float get_signed_distance_from_plane(vec3 p, vec3 eqn) {
        // dot(eqn.xyz,p) + eqn.w, , except we know eqn.w is zero 
        // (see CreatePlaneEquation above)
        return dot(eqn,p);
    }

    bool test_frustum_sides(vec3 c, float r, vec3 plane0, vec3 plane1, vec3 plane2, vec3 plane3) {
        bool intersecting = get_signed_distance_from_plane(c, plane0) < r;
        bool intersecting1 = get_signed_distance_from_plane(c, plane1) < r;
        bool intersecting2 = get_signed_distance_from_plane(c, plane2) < r;
        bool intersecting3 = get_signed_distance_from_plane(c, plane3) < r;

        return (intersecting && intersecting1 && 
                intersecting2 && intersecting3);
    }

    vec4 convert_proj_to_view(vec4 p) {
        p = inv_proj_view * p;
        p /= p.w;
        return p;
    }

    float convert_proj_depth_to_view(float z) {
        z = -1.f / (z * inv_proj_view[2][3] + inv_proj_view[3][3]);
        return z;
    }

    void calc_min_max_z(uvec3 global_invocation_id) {
        ivec2 idx = ivec2(global_invocation_id.x, global_invocation_id.y);
        float depth = texelFetch(depth_sampler, idx, 0).x;
        /* float view_z = convert_proj_depth_to_view(depth); */
        uint z = floatBitsToUint(-depth);

        if (depth != 0.f) {
            atomicMin(min_depth_uint, z);
            atomicMax(max_depth_uint, z);
        }
    }

    layout(local_size_x = TILE_SIZE, local_size_y = TILE_SIZE) in;
    void main()
    {
        uint index = gl_WorkGroupID.y * gl_NumWorkGroups.x + gl_WorkGroupID.x;

        if (gl_LocalInvocationIndex == 0)
        {
            min_depth_uint = 0x7f7fffff;
            max_depth_uint = 0;
            visible_light_count = 0;
            proj_view = cam.proj * cam.view;
            inv_proj_view = inverse(proj_view);
            inv_proj = inverse(cam.proj);
            visible_lights_buffer.num_tiles_x = gl_NumWorkGroups.x;
        }

        barrier();

        vec3 frustum_eqn_0, frustum_eqn_1, frustum_eqn_2, frustum_eqn_3;
        {
            // Calculate position of the corners of this group's tile in raster space
            uint rast_xt = TILE_SIZE * gl_WorkGroupID.x;
            uint rast_yt = TILE_SIZE * gl_WorkGroupID.y;
            uint rast_xb = TILE_SIZE * (gl_WorkGroupID.x + 1);
            uint rast_yb = TILE_SIZE * (gl_WorkGroupID.y + 1);

            vec2 texture_size = textureSize(depth_sampler, 0);
            float raster_width = texture_size.x;
            float raster_height = texture_size.y;

            // Calculate position of the corners of this group's tile in NDC space
            // Different from the AMD DX 11 sample because Vulkan's NDC space has
            // the Y axis pointing downwards
            vec4 top_left = vec4(
                    float(rast_xt) / raster_width * 2.f - 1.f,
                    float(rast_yt) / raster_height * 2.f - 1.f,
                    1.f,
                    1.f);
            vec4 bottom_left = vec4(
                    top_left.x,
                    float(rast_yb) / raster_height * 2.f - 1.f,
                    1.f,
                    1.f);
            vec4 bottom_right = vec4(
                    float(rast_xb) / raster_width * 2.f - 1.f,
                    bottom_left.y,
                    1.f,
                    1.f);
            vec4 top_right = vec4(
                    bottom_right.x,
                    top_left.y,
                    1.f,
                    1.f);

            // Convert the four corners from NDC to view space
            vec3 top_left_vs = convert_proj_to_view(top_left).xyz;
            vec3 bottom_left_vs = convert_proj_to_view(bottom_left).xyz;
            vec3 bottom_right_vs = convert_proj_to_view(bottom_right).xyz;
            vec3 top_right_vs = convert_proj_to_view(top_right).xyz;

            // Create plane equations for the four sides of the frustum,
            // with the positive half space outside the frustum. Also
            // change the handedness from left to right compared to the AMD
            // DX 11 sample, since we chose the view space to be right handed
            frustum_eqn_0 = create_plane_eqn(top_right.xyz, top_left.xyz);
            frustum_eqn_1 = create_plane_eqn(top_left.xyz, bottom_left.xyz);
            frustum_eqn_2 = create_plane_eqn(bottom_left.xyz, bottom_right.xyz);
            frustum_eqn_3 = create_plane_eqn(bottom_right.xyz, top_right.xyz);
        }

        barrier();

        calc_min_max_z(gl_GlobalInvocationID);
        
        barrier();

        float min_z = -(uintBitsToFloat(min_depth_uint));
        float max_z = -(uintBitsToFloat(max_depth_uint));

        min_z = linearize_depth(min_z);
        max_z = linearize_depth(max_z);

        // Cull lights
        const uint threads_per_tile = TILE_SIZE * TILE_SIZE;
        for (uint i = gl_LocalInvocationIndex; i < env.light_count; i += threads_per_tile) {
            vec4 light_center = (proj_view * env.point_lights[i].pos);
            light_center.xyz /= light_center.w;
            float light_radius = env.point_lights[i].radius;
            if (test_frustum_sides(light_center.xyz, light_radius,
                        frustum_eqn_0, frustum_eqn_1, frustum_eqn_2, frustum_eqn_3)) {
                if (max_z - linearize_depth(light_center.z) < light_radius && 
                        linearize_depth(light_center.z) - min_z < light_radius) {
                    uint dst_idx = atomicAdd(visible_light_count, 1);
                    visible_light_indices[dst_idx] = int(i);
                }
            }
        }

        barrier();

        if (gl_LocalInvocationIndex == 0) {
            uint offset = index * MAX_POINT_LIGHTS;
            for (uint i = 0; i < visible_light_count; i++) {
                visible_lights_buffer.indices[offset + i] = visible_light_indices[i];
            }

            if (visible_light_count != MAX_POINT_LIGHTS) {
                visible_lights_buffer.indices[offset + visible_light_count] = -1;
            }
        }
    }
}@
