compute = @{
    #include "common.glsl"

    layout (set = 0, binding = 0) uniform sampler2D depth_sampler;

    layout (set = 0, binding = 1) uniform EnvironmentUniform {
        Environment environment;
    };

    layout (set = 0, binding = 2) writeonly buffer VisibleLightsBuffer {
        uint indices[];
    };

    const uint TILE_SIZE = 16;

    shared uint min_depth;
    shared uint max_depth;

    layout(local_size_x = TILE_SIZE, local_size_y = TILE_SIZE, local_size_z = 1) in;
    void main()
    {
        if (gl_LocalInvocationIndex == 0)
        {
            min_depth = 0xFFFFFF;
            max_depth = 0;
        }

        barrier();

        vec2 tex_coord = vec2(gl_GlobalInvocationID.xy) / textureSize(depth_sampler, 0);
        float depth_float = texture(depth_sampler, tex_coord).x;
        uint depth_uint = floatBitsToUint(depth_float);

        min_depth = atomicMin(min_depth, depth_uint);
        max_depth = atomicMax(max_depth, depth_uint);
        
        barrier();

    }
}@
