#version 450

layout(location = 0) in vec3 inWorldPos;
layout(location = 1) in vec2 inUV;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec3 inTangent;
layout(location = 4) in vec3 inBitangent;

layout(location = 0) out vec4 outColor;

#include "common/bindless.glsl" 
#include "common/environment.glsl"
#include "common/primitive_types.glsl"

#include "material/principled_bsdf.glsl"

layout(set = 0, binding = 0) uniform Camera { mat4 view; mat4 proj; vec3 position; } camera;
layout(set = 0, binding = 1) uniform Environment { EnvironmentParams params; } env;

layout(std430, set = 3, binding = 0) readonly buffer BSDFMaterials { 
  PrincipledBSDFData materials[]; 
} bsdfBlock;

layout(push_constant) uniform PushConstants {
  mat4 model;
  Primitive prim;
  uint materialIndex;
} pcs;

void main() {
  PrincipledBSDFData mat = bsdfBlock.materials[pcs.materialIndex];

  vec3 N = normalize(inNormal);
  vec3 T = length(inTangent) > 0.1 ? normalize(inTangent) : vec3(1.0, 0.0, 0.0);
  vec3 B = length(inBitangent) > 0.1 ? normalize(inBitangent) : cross(N, T);
  mat3 baseTBN = mat3(T, B, N);

  vec3 V = normalize(camera.position - inWorldPos);
  
  Surface s = buildSurface(inWorldPos, V, baseTBN, inUV, mat);
  
  if (s.albedo.a < 0.01) discard;

  vec3 L = V;
  
  vec3 finalColor = evaluateUberLighting(s, L, baseTBN, mat, env.params);

  outColor = vec4(finalColor, s.albedo.a);
}
