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

    shared uint min_depth_uint;
    shared uint max_depth_uint;
    shared uint visible_light_count;
    shared int visible_light_indices[MAX_POINT_LIGHTS];
    shared mat4 view_projection;
    shared vec4 frustum_planes[6];

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

        vec2 tex_coord = vec2(gl_GlobalInvocationID.xy) / textureSize(depth_sampler, 0);
        float depth_float = texture(depth_sampler, tex_coord).x;
        // Linearize depth because of perspective projection
        depth_float = (0.5 * cam.proj[3][2]) / (depth_float + 0.5 * cam.proj[2][2] - 0.5);

        uint depth_uint = floatBitsToUint(depth_float);
        min_depth_uint = atomicMin(min_depth_uint, depth_uint);
        max_depth_uint = atomicMax(max_depth_uint, depth_uint);
        
        barrier();

        // Calculate the frustum planes
        if (gl_LocalInvocationIndex == 0)
        {
            float min_depth;
            float max_depth;

            // Convert the min and max across the entire tile back to float
            min_depth = uintBitsToFloat(min_depth_uint);
            max_depth = uintBitsToFloat(max_depth_uint);

            if (min_depth >= max_depth)
            {
                min_depth = max_depth;
            }


            // Steps based on tile sale
            vec2 negative_step = (2.0 * vec2(gl_WorkGroupID.xy)) / vec2(gl_NumWorkGroups.xy);
            vec2 positive_step =
                (2.0 * vec2(gl_WorkGroupID.xy + ivec2(1, 1))) / vec2(gl_NumWorkGroups.xy);

            // Set up starting values for planes using steps and min and max z values
            frustum_planes[0] = vec4(1.0, 0.0, 0.0, 1.0 - negative_step.x); // Left
            frustum_planes[1] = vec4(-1.0, 0.0, 0.0, -1.0 + positive_step.x); // Right
            frustum_planes[2] = vec4(0.0, 1.0, 0.0, 1.0 - negative_step.y); // Bottom
            frustum_planes[3] = vec4(0.0, -1.0, 0.0, -1.0 + positive_step.y); // Top
            frustum_planes[4] = vec4(0.0, 0.0, 0.0, -min_depth); // Near
            frustum_planes[5] = vec4(0.0, 0.0, 1.0, max_depth); // Far

            // Transform the first four planes
            for (uint i = 0; i < 4; i++) {
                frustum_planes[i] *= view_projection;
                frustum_planes[i] /= length(frustum_planes[i].xyz);
            }

            // Transform the depth planes
            frustum_planes[4] *= cam.view;
            frustum_planes[4] /= length(frustum_planes[4].xyz);
            frustum_planes[5] *= cam.view;
            frustum_planes[5] /= length(frustum_planes[5].xyz);
        }

        barrier();

        // Cull lights
        uint thread_count = TILE_SIZE * TILE_SIZE;
        uint pass_count = (env.light_count + thread_count - 1) / thread_count;
        for (uint i = 0; i < pass_count; i++) {
            // Get the lightIndex to test for this thread / pass. If the index is >= light count, then this thread can stop testing lights
            uint light_index = i * thread_count + gl_LocalInvocationIndex;
            if (light_index >= env.light_count) {
                break;
            }

            vec4 position = env.point_lights[light_index].pos;
            float radius = env.point_lights[light_index].radius;

            // We check if the light exists in our frustum
            float distance = 0.0;
            for (uint j = 0; j < 6; j++) {
                distance = dot(position, frustum_planes[j]) + radius;

                // If one of the tests fails, then there is no intersection
                if (distance <= 0.0) {
                    break;
                }
            }

            // If greater than zero, then it is a visible light
            if (distance > 0.0) {
                // Add index to the shared array of visible indices
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
