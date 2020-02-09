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
    shared mat4 view_projection;
    shared ViewFrustum frustum;

    ViewFrustum create_frustum(ivec2 tile_pos)
    {
        mat4 inv_view_proj = inverse(view_projection);
        vec2 ndc_size_per_tile = 2.0 * vec2(TILE_SIZE, TILE_SIZE) / vec2(textureSize(depth_sampler, 0).xy);

        // 2D corners of tile in NDC
        vec2 ndc_pts[4];
        ndc_pts[0] = ndc_upper_left + tile_pos * ndc_size_per_tile; // upper left
        ndc_pts[1] = vec2(ndc_pts[0].x + ndc_size_per_tile.x, ndc_pts[0].y); // upper right
        ndc_pts[2] = ndc_pts[0] + ndc_size_per_tile; // lower right
        ndc_pts[3] = vec2(ndc_pts[0].x, ndc_pts[0].y + ndc_size_per_tile.y); // lower left

        float min_depth = uintBitsToFloat(min_depth_uint);
        float max_depth = uintBitsToFloat(max_depth_uint);

        ViewFrustum frustum;

        // Get the points
        vec4 temp;
        for (int i = 0; i < 4; i++)
        {
            temp = inv_view_proj * vec4(ndc_pts[i], min_depth, 1.0);
            frustum.points[i] = temp.xyz / temp.w;
            temp = inv_view_proj * vec4(ndc_pts[i], max_depth, 1.0);
            frustum.points[i+4] = temp.xyz / temp.w;
        }

        vec3 temp_normal;
        for (int i = 0; i < 4; i++)
        {
            temp_normal = cross(frustum.points[i] - cam.pos.xyz, frustum.points[i+1] - cam.pos.xyz);
            temp_normal = normalize(temp_normal);
            frustum.planes[i] = vec4(temp_normal, -dot(temp_normal, frustum.points[i]));
        }

        // near plane
        {
            temp_normal = cross(frustum.points[1] - frustum.points[0], frustum.points[3] - frustum.points[0]);
            temp_normal = normalize(temp_normal);
            frustum.planes[4] = vec4(temp_normal, -dot(temp_normal, frustum.points[0]));
        }

        // far plane
        {
            temp_normal = cross(frustum.points[7] - frustum.points[4], frustum.points[5] - frustum.points[4]);
            temp_normal = normalize(temp_normal);
            frustum.planes[5] = vec4(temp_normal, -dot(temp_normal, frustum.points[4]));
        }

        return frustum;
    }

    bool is_collided(PointLight light, ViewFrustum frustum)
    {
        bool result = true;
        for (int i = 0; i < 4; i++)
        {
            if (dot(light.pos.xyz, frustum.planes[i].xyz) + frustum.planes[i].w < -light.radius)
            {
                result = false;
                break;
            }
        }

        if (!result)
        {
            return false;
        }

        // Step2: bbox corner test (to reduce false positive)
        vec3 light_bbox_max = light.pos.xyz + vec3(light.radius);
        vec3 light_bbox_min = light.pos.xyz - vec3(light.radius);
        int probe;
        probe = 0;
        for (int i = 0; i < 8; i++) probe += ((frustum.points[i].x > light_bbox_max.x) ? 1 : 0); if(probe == 8) return false;
        probe = 0;
        for (int i = 0; i < 8; i++) probe += ((frustum.points[i].x < light_bbox_min.x) ? 1 : 0); if(probe == 8) return false;
        probe = 0;
        for (int i = 0; i < 8; i++) probe += ((frustum.points[i].y > light_bbox_max.y) ? 1 : 0); if(probe == 8) return false;
        probe = 0;
        for (int i = 0; i < 8; i++) probe += ((frustum.points[i].y < light_bbox_min.y) ? 1 : 0); if(probe == 8) return false;
        probe = 0;
        for (int i = 0; i < 8; i++) probe += ((frustum.points[i].z > light_bbox_max.z) ? 1 : 0); if(probe == 8) return false;
        probe = 0;
        for (int i = 0; i < 8; i++) probe += ((frustum.points[i].z < light_bbox_min.z) ? 1 : 0); if(probe == 8) return false;

        return true;
    }

    layout(local_size_x = TILE_SIZE, local_size_y = TILE_SIZE) in;
    void main()
    {
        uint index = gl_WorkGroupID.y * gl_NumWorkGroups.x + gl_WorkGroupID.x;

        if (gl_LocalInvocationIndex == 0)
        {
            min_depth_uint = 0xFFFFFF;
            max_depth_uint = 0;
            visible_light_count = 0;
            view_projection = cam.proj * cam.view;
            visible_lights_buffer.num_tiles_x = gl_NumWorkGroups.x;
        }

        barrier();

        float depth_float = texelFetch(depth_sampler, ivec2(gl_GlobalInvocationID.xy), 0).x;
        // Linearize depth because of perspective projection
        depth_float = (0.5 * cam.proj[3][2]) / (depth_float + 0.5 * cam.proj[2][2] - 0.5);

        uint depth_uint = floatBitsToUint(depth_float);
        min_depth_uint = atomicMin(min_depth_uint, depth_uint);
        max_depth_uint = atomicMax(max_depth_uint, depth_uint);
        
        barrier();

        // Calculate the frustum planes
        if (gl_LocalInvocationIndex == 0)
        {
            frustum = create_frustum(ivec2(gl_WorkGroupID.xy));
        }

        barrier();

        // Cull lights
        uint light_index = gl_LocalInvocationIndex;
        if (light_index < env.light_count) {
            if (is_collided(env.point_lights[light_index], frustum))
            {
                uint offset = atomicAdd(visible_light_count, 1);
                visible_light_indices[offset] = int(light_index);
            }
        }

        barrier();

        // One thread should fill the global light buffer
        if (gl_LocalInvocationIndex == 0) {
            uint offset = index * MAX_POINT_LIGHTS; // Determine bosition in global buffer
            for (uint i = 0; i < visible_light_count; i++) {
                visible_lights_buffer.indices[offset + i] = visible_light_indices[i];
            }

            if (visible_light_count != MAX_POINT_LIGHTS) {
                // Unless we have totally filled the entire array, mark it's end with -1
                // Final shader step will use this to determine where to stop (without having to pass the light count)
                visible_lights_buffer.indices[offset + visible_light_count] = -1;
            }
        }
    }
}@
