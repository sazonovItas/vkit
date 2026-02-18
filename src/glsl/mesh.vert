#version 450 core

#include "mesh_pcs.glsl"

layout (set = 0, binding = 0) uniform View {
  mat4 mat_vp;
};

layout (set = 2, binding = 0) readonly buffer Instances {
  mat4 mat_ms[];
};

layout (location = 0) out vec3 out_pos;
layout (location = 1) out vec2 out_uv;
layout (location = 2) out vec3 out_normal;
layout (location = 3) out vec4 out_tangent;

void main() {
  Vertex v = pcs.vertex_buffer.vertices[gl_VertexIndex];

  const mat4 mat_m = mat_ms[gl_InstanceIndex];
  const vec4 world_pos = mat_m * vec4(v.position, 1.0);

  gl_Position = mat_vp * world_pos;
  
  out_pos = world_pos.xyz;
  out_uv = vec2(v.uv_x, v.uv_y);
  out_normal = normalize(mat3(mat_m) * v.normal);
  out_tangent = v.tangent;
}
