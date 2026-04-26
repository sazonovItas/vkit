#version 450 core

layout(location = 0) in vec3 inViewDir;

layout(location = 0) out vec4 outColor;

#include "common/bindless.glsl"
#include "common/environment.glsl"

layout(set = 0, binding = 2) uniform Environment { EnvironmentParams params; } env;

layout(push_constant) uniform PushConstants {
  vec4  baseColor;
  float blurLevel;
} pcs;

const vec2 INV_ATAN = vec2(0.15915494309, 0.31830988618);

vec2 directionToSphericalEnvmap(vec3 dir) {
  vec2 uv = vec2(atan(dir.z, dir.x), asin(dir.y));
  uv *= INV_ATAN;
  uv += 0.5;
  uv.y = 1.0 - uv.y;
  return uv;
}

void main() {
  vec3 viewDir = normalize(inViewDir);
  vec3 envColor = pcs.baseColor.rgb;

  if (env.params.prefilterTexIdx >= 0) {
    vec2 uv = directionToSphericalEnvmap(viewDir);
    
    float rawL = clamp(pcs.blurLevel, 0.0, 1.0) * float(env.params.prefilterNumLayers - 1);
    
    vec3 prefFloor = sampleTexture2DArrayLinear(uint(env.params.prefilterTexIdx), vec3(uv, floor(rawL))).rgb;
    vec3 prefCeil  = sampleTexture2DArrayLinear(uint(env.params.prefilterTexIdx), vec3(uv, ceil(rawL))).rgb;
    envColor = mix(prefFloor, prefCeil, fract(rawL));
  }

  envColor *= env.params.intensity;

  outColor = vec4(envColor, 1.0);
}
