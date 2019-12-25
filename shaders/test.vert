#version 450

const vec3[3] verts = {
  vec3(0.5f, 0.5f, 0.0f),
  vec3(-0.5f, 0.5f, 0.0f),
  vec3(-0.5f, -0.5f, 0.0f)
};

void main() {
  gl_Position = vec4(verts[gl_VertexIndex], 1.0f);
}
