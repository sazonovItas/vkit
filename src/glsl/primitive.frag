#version 450 core

#include "pcs.glsl"
#include "material.glsl"
#include "bindless.glsl"

layout (location = 0) in vec3 inWorldPos;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inUV0;
layout (location = 3) in vec2 inUV1;
layout (location = 4) in vec3 inColor0;

layout (set = 0, binding = 0) uniform UBO {
  mat4 model;
  mat4 view;
  mat4 projection;
  vec3 cameraPosition;
} ubo;

layout (set = 0, binding = 1) uniform UBOParams {
  vec3 lightDir;
} uboParams;

layout (set = 1, binding = 0, std430) readonly buffer SSBO {
  Material materials[];
};

layout (location = 0) out vec4 outColor;

void main() {
  Material material = materials[pcs.materialIdx];

  vec4 baseColor;
  if (material.baseColorTextureIdx != -1) {
    baseColor = sampleTexture2DLinear(material.baseColorTextureIdx, inUV0);
  } else {
    baseColor = material.baseColorFactor;
  }

  vec3 normal = normalize(inNormal);
  float diff = max(dot(normal, -uboParams.lightDir), 0.0);

  outColor = vec4(inColor0 * diff, 1.0) * baseColor;
}
