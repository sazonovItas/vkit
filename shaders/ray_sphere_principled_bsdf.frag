#version 450

layout(location = 0) in vec3 inLocalPos;
layout(location = 1) in vec3 inWorldPos;

layout(location = 0) out vec4 outColor;

#include "ray_sphere.glsl"

#include "common/bindless.glsl" 
#include "common/environment.glsl"

#include "material/principled_bsdf.glsl"

layout(set = 0, binding = 0) uniform Camera { mat4 view; mat4 proj; vec3 position; } camera;
layout(set = 0, binding = 1) uniform Environment { EnvironmentParams params; } env;

layout(std430, set = 2, binding = 2) readonly buffer BSDFMaterials { 
  PrincipledBSDFData materials[]; 
} bsdfBlock;

layout(push_constant) uniform PushConstants {
  mat4 model;
  uint materialIndex;
} pcs;

void main() {
  SphereHit hit = calculateSphereHit(inLocalPos, camera.position, pcs.model, camera.view, camera.proj);

  PrincipledBSDFData mat = bsdfBlock.materials[pcs.materialIndex];

  mat3 baseTBN = mat3(hit.tangent, hit.bitangent, hit.normal);

  vec3 V = normalize(camera.position - hit.worldPos);
  
  Surface s = buildSurface(hit.worldPos, V, baseTBN, hit.uv, mat);
  
  if (s.albedo.a < 0.01) discard;

  vec3 L = V; 
  vec3 finalColor = evaluateUberLighting(s, L, baseTBN, mat, env.params);

  outColor = vec4(finalColor, s.albedo.a);
}
