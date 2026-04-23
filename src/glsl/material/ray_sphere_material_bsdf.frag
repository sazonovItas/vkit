#version 450

layout(location = 0) in vec3 inWorldPos;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform Camera {
  mat4 view;
  mat4 proj;
  vec3 position;
} camera;

#include "environment_layouts.glsl"
#include "material_bsdf_layouts.glsl"

#include "material_bsdf_layers.glsl"

layout(push_constant) uniform PushConstants { 
  mat4 model; 
} pcs;

vec3 getSphereCenter() { return pcs.model[3].xyz; }

float getSphereRadius() { return pcs.model[0][0] * 0.5; }

float intersectSphere(vec3 origin, vec3 dir, vec3 center, float radius) {
  vec3 oc = origin - center;
  float b = dot(oc, dir);
  float c = dot(oc, oc) - radius * radius;
  float h = b * b - c;
  return (h < 0.0) ? -1.0 : -b - sqrt(h);
}

vec2 getSphereUV(vec3 n) {
  return vec2(0.5 + (atan(n.z, n.x) / (2.0 * PI)), 0.5 - (asin(n.y) / PI));
}

mat3 getSphereTBN(vec3 N) {
  vec3 up = abs(N.y) < 0.999 ? vec3(0.0, 1.0, 0.0) : vec3(0.0, 0.0, 1.0);
  vec3 tangent = normalize(cross(up, N));
  return mat3(tangent, cross(N, tangent), N);
}

void main() {
  vec3 rayOrigin = camera.position;
  vec3 rayDir = normalize(inWorldPos - rayOrigin);
  vec3 center = getSphereCenter();

  float t = intersectSphere(rayOrigin, rayDir, center, getSphereRadius());
  if (t < 0.0) discard; 

  vec3 hitPos = rayOrigin + t * rayDir;
  
  vec4 clipPos = camera.proj * camera.view * vec4(hitPos, 1.0);
  gl_FragDepth = (clipPos.z / clipPos.w);

  vec3 geomNormal = normalize(hitPos - center);
  vec2 uv = getSphereUV(geomNormal);
  mat3 TBN = getSphereTBN(geomNormal);
  vec3 V = -rayDir;
  
  Surface s = buildSurface(hitPos, geomNormal, uv, V, TBN);
  vec3 L = normalize(vec3(1.0, 1.0, 1.0)); 
  vec3 color = evaluateUberLighting(s, L, TBN);

  outColor = vec4(color / (color + vec3(1.0)), mat.baseColorFactor.a);
}
