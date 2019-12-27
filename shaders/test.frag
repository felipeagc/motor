#version 450

layout (set = 0, binding = 0) uniform MyUniform {
  float c;
};

layout (location = 0) out vec4 out_color;

void main() {
  out_color = vec4(vec3(c), 1.0f);
}
