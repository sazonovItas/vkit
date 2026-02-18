#version 450 core

layout (location = 0) in vec3 in_pos;
layout (location = 1) in vec2 in_uv;
layout (location = 2) in vec3 in_normal;
layout (location = 3) in vec3 in_tangent;

layout (set = 1, binding = 0) uniform sampler2D tex;

layout (location = 0) out vec4 out_color;

void main() {
  vec3 in_color = vec3(1.0);
  vec3 normal = -normalize(in_normal);

  vec3 light_dir = normalize(vec3(0.0, 0.0, -1.0));
  float diff = max(dot(normal, light_dir), 0.0) * 0.5 + 0.5;

  out_color = vec4(in_color * diff, 1.0) * texture(tex, in_uv);
  // out_color = vec4(in_color * diff, 1.0);
}
