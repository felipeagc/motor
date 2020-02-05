compute = @{
    const uint TILE_SIZE = 16;

    layout(local_size_x = TILE_SIZE, local_size_y = TILE_SIZE, local_size_z = 1) in;

    layout (set = 0, binding = 0) uniform sampler2D depth_sampler;

    void main()
    {
        float depth = texture(depth_sampler, vec2(0.0, 0.0)).x;
    }
}@
