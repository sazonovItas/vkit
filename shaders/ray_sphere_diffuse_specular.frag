#version 450

layout(location = 0) in vec3 inLocalPos;
layout(location = 1) in vec3 inWorldPos;

layout(location = 0) out vec4 outColor;

#include "ray_sphere.glsl"
#include "common/bindless.glsl" 

#include "material/diffuse_specular.glsl"

const vec3 LIGHT_COLOR = vec3(1.0, 1.0, 1.0);
const vec3 AMBIENT_COLOR = vec3(0.05, 0.05, 0.05);

layout(set = 0, binding = 0) uniform Camera { mat4 view; mat4 proj; vec3 position; } camera;

layout(std430, set = 2, binding = 1) readonly buffer DiffuseSpecularMaterials { 
  DiffuseSpecularData materials[]; 
} diffSpecBlock;

layout(push_constant) uniform PushConstants {
  mat4 model;
  uint materialIndex;
} pcs;

void main() {
  SphereHit hit = calculateSphereHit(inLocalPos, camera.position, pcs.model, camera.view, camera.proj);

  DiffuseSpecularData mat = diffSpecBlock.materials[pcs.materialIndex];

  vec4 albedo = mat.params.diffuseFactor;
  if (mat.textures.diffuseTexIdx >= 0) {
    albedo *= sampleTexture2DLinear(uint(mat.textures.diffuseTexIdx), hit.uv);
  }

  if (albedo.a < 0.01) discard;

  vec3 N = hit.normal;
  if (mat.textures.normalTexIdx >= 0) {
    mat3 TBN = mat3(hit.tangent, hit.bitangent, N);
    vec3 normalSample = sampleTexture2DLinear(uint(mat.textures.normalTexIdx), hit.uv).xyz;
    N = normalize(TBN * (normalSample * 2.0 - 1.0));
  }

  vec3 specularColor = mat.params.specularFactor;
  float glossiness = mat.params.glossinessFactor;

  if (mat.textures.specularGlossinessTexIdx >= 0) {
    vec4 specGlossMap = sampleTexture2DLinear(uint(mat.textures.specularGlossinessTexIdx), hit.uv);
    specularColor *= specGlossMap.rgb;
    glossiness *= specGlossMap.a;
  }

  float shininess = exp2(glossiness * 10.0 + 1.0);

  vec3 V = normalize(camera.position - hit.worldPos);
  vec3 L = V; 
  vec3 H = normalize(L + V); 
  
  float NdotL = max(dot(N, L), 0.0);
  float NdotH = max(dot(N, H), 0.0);

  vec3 diffuseLighting = albedo.rgb * NdotL;
  
  vec3 specularLighting = vec3(0.0);
  if (NdotL > 0.0) {
    specularLighting = specularColor * pow(NdotH, shininess);
  }

  vec3 finalColor = (AMBIENT_COLOR * albedo.rgb) + (LIGHT_COLOR * (diffuseLighting + specularLighting));

  outColor = vec4(finalColor, albedo.a);
}
