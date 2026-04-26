#version 450

layout(location = 0) in vec3 inWorldPos;
layout(location = 1) in vec2 inUV;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec3 inTangent;
layout(location = 4) in vec3 inBitangent;

layout(location = 0) out vec4 outColor;

#include "common/bindless.glsl" 
#include "common/primitive_types.glsl"   

#include "material/diffuse.glsl"

const vec3 LIGHT_COLOR = vec3(1.0, 1.0, 1.0);
const vec3 AMBIENT_COLOR = vec3(0.05, 0.05, 0.05);

layout(set = 0, binding = 0) uniform Camera { mat4 view; mat4 proj; vec3 position; } camera;

layout(std430, set = 2, binding = 0) readonly buffer DiffuseMaterials { 
  DiffuseData materials[]; 
} diffuseBlock;

layout(push_constant) uniform PushConstants {
  mat4 model;
  Primitive prim;
  uint materialIndex;
} pcs;

void main() {
  DiffuseData mat = diffuseBlock.materials[pcs.materialIndex];

  vec4 albedo = mat.params.diffuseFactor;
  if (mat.textures.diffuseTexIdx >= 0) {
    albedo *= sampleTexture2DLinear(uint(mat.textures.diffuseTexIdx), inUV);
  }

  if (albedo.a < 0.01) discard;

  vec3 N = normalize(inNormal);
  if (mat.textures.normalTexIdx >= 0) {
    vec3 T = length(inTangent) > 0.1 ? normalize(inTangent) : vec3(1.0, 0.0, 0.0);
    vec3 B = length(inBitangent) > 0.1 ? normalize(inBitangent) : cross(N, T);
    mat3 TBN = mat3(T, B, N);

    vec3 normalSample = sampleTexture2DLinear(uint(mat.textures.normalTexIdx), inUV).xyz;
    N = normalize(TBN * (normalSample * 2.0 - 1.0));
  }

  vec3 V = normalize(camera.position - inWorldPos);
  vec3 L = V; 
  
  float NdotL = max(dot(N, L), 0.0);
  vec3 diffuseLighting = (AMBIENT_COLOR + (LIGHT_COLOR * NdotL)) * albedo.rgb;

  outColor = vec4(diffuseLighting, albedo.a);
}
