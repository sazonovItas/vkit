#version 450

layout(location = 0) in vec3 inWorldPos;
layout(location = 1) in vec2 inUV;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec3 inTangent;
layout(location = 4) in vec3 inBitangent;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform Camera {
  mat4 view;
  mat4 proj;
  vec3 position;
} camera;

#include "environment_layouts.glsl"
#include "material_bsdf_layouts.glsl"

#include "material_bsdf_layers.glsl"

void main() {
  vec3 geomNormal = normalize(inNormal);
  vec3 rawTangent = length(inTangent) > 0.1 ? normalize(inTangent) : vec3(1.0, 0.0, 0.0);
  vec3 rawBitangent = length(inBitangent) > 0.1 ? normalize(inBitangent) : cross(geomNormal, rawTangent);
  mat3 TBN = mat3(rawTangent, rawBitangent, geomNormal);
  
  vec3 V = normalize(camera.position - inWorldPos);

  Surface s = buildSurface(inWorldPos, geomNormal, inUV, V, TBN);
  vec3 L = normalize(vec3(1.0, 1.0, 1.0));
  vec3 color = evaluateUberLighting(s, L, TBN);

  outColor = vec4(color / (color + vec3(1.0)), mat.baseColorFactor.a);
}
