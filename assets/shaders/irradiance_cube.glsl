blending = false
depth_test = false
depth_write = false
cull_mode = "none"
front_face = "clockwise"

vertex = @{
    layout(location = 0) in vec3 pos;

    layout(set = 0, binding = 0) uniform IrradianceUniform {
        mat4 mvp;
        float deltaPhi;
        float deltaTheta;
    };

    layout(location = 0) out vec3 tex_coords0;

    void main() {
        tex_coords0 = pos;
        tex_coords0.y *= -1.0f;

        gl_Position = mvp * vec4(pos, 1.0);
    }
}@

fragment = @{
    layout (location = 0) in vec3 inPos;

    layout (location = 0) out vec4 outColor;

    layout(set = 0, binding = 0) uniform IrradianceUniform {
        mat4 mvp;
    } consts;
    layout (set = 0, binding = 1) uniform samplerCube samplerEnv;

    #define PI 3.1415926535897932384626433832795

    void main() {
        vec3 N = normalize(inPos);
        vec3 up = vec3(0.0, 1.0, 0.0);
        vec3 right = normalize(cross(up, N));
        up = cross(N, right);

        const float TWO_PI  = PI * 2.0;
        const float HALF_PI = PI * 0.5;

        const float delta_phi   = (2.0f * PI) / 180.0f;
        const float delta_theta = (0.5f * PI) / 64.0f;

        vec3 color = vec3(0.0);
        uint sampleCount = 0u;
        for (float phi = 0.0; phi < TWO_PI; phi += delta_phi) {
            for (float theta = 0.0; theta < HALF_PI; theta += delta_theta) {
                vec3 tempVec = cos(phi) * right + sin(phi) * up;
                vec3 sampleVector = cos(theta) * N + sin(theta) * tempVec;
                color += texture(samplerEnv, sampleVector).rgb * cos(theta) * sin(theta);
                sampleCount++;
            }
        }
        outColor = vec4(PI * color / float(sampleCount), 1.0);
    }
}@
