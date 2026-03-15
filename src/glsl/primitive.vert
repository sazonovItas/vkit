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

void main() {
  Vertex v = pcs.vertices.data[gl_VertexIndex];

  mat4 model = ubo.model * pcs.transform;
  vec4 worldPos = model * vec4(v.position, 1.0);
  
  mat3 normalMatrix = transpose(inverse(mat3(model)));
  vec3 normal = normalize(normalMatrix * v.normal);

  outWorldPos = worldPos.xyz;
  outNormal = normal;
  outUV0 = vec2(v.uvX, v.uvY);

  gl_Position = ubo.projection * ubo.view * worldPos;
}
