#version 450 core

#include "pcs.glsl"

layout (set = 0, binding = 0) uniform UBO {
  mat4 model;
  mat4 view;
  mat4 projection;
  vec3 cameraPosition;
} ubo;

layout (location = 0) out vec3 outWorldPos;
layout (location = 1) out vec3 outNormal;
layout (location = 2) out vec2 outUV0;
layout (location = 3) out vec2 outUV1;
layout (location = 4) out vec3 inColor0;

void main() {
  Vertex v = pcs.vertices.data[gl_VertexIndex];

  vec4 worldPos = ubo.model * vec4(v.position, 1.0);
  vec3 normal = normalize(transpose(inverse(mat3(ubo.model))) * v.normal);
  vec2 uv = vec2(v.uvX, v.uvY);

  outWorldPos = worldPos.xyz;
  outNormal = normal;
  outUV0 = uv;
  outUV1 = uv;
  inColor0 = vec3(1.0);

  gl_Position = ubo.projection * ubo.view * worldPos;
}
