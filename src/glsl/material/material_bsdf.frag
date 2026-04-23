#version 450

#include "math_bsdf.glsl"

layout(location = 0) in vec3 inWorldPos;
layout(location = 1) in vec2 inUV;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec3 inTangent;
layout(location = 4) in vec3 inBitangent;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform Scene { uint dummy; } sceneParams;
layout(set = 0, binding = 1) uniform Camera { mat4 view; mat4 proj; vec3 position; } camera;

layout(set = 1, binding = 0) uniform sampler2D      irradianceMap;
layout(set = 1, binding = 1) uniform sampler2DArray prefilterMapArray;
layout(set = 1, binding = 2) uniform Environment {
  vec3  ambientColor;
  float intensity;
  uint  hasIrradiance;
  uint  hasPrefilter;
  uint  prefilterLayers;
} env;

layout(set = 3, binding = 0) uniform Material {
  vec4  baseColorFactor;
  vec3  emissiveFactor;
  float metallicFactor;
  float roughnessFactor;

  float ior;
  float clearcoatFactor;
  float clearcoatRoughnessFactor;
  float transmissionFactor;

  uint  hasBaseColorMap;
  uint  hasNormalMap;
  uint  hasMetallicRoughnessMap;
  uint  hasEmissiveMap;
  uint  hasClearcoatMap;
  uint  hasClearcoatNormalMap;
} mat;

layout(set = 3, binding = 1) uniform sampler2D baseColorMap;
layout(set = 3, binding = 2) uniform sampler2D normalMap;
layout(set = 3, binding = 3) uniform sampler2D metallicRoughnessMap;
layout(set = 3, binding = 4) uniform sampler2D emissiveMap;
layout(set = 3, binding = 5) uniform sampler2D clearcoatMap;
layout(set = 3, binding = 6) uniform sampler2D clearcoatNormalMap;

struct Surface {
  vec3  N, V, R, F0, albedo;
  float roughness, metallic, dotNV;
};

vec3 evaluateDirectPBR(Surface s, vec3 L) {
  vec3 H = normalize(s.V + L);
  float dotNL = clamp01(dot(s.N, L));
  float dotNH = clamp01(dot(s.N, H));
  float dotVH = clamp01(dot(s.V, H));

  float D = D_GGX(dotNH, s.roughness);
  float G = G_SchlickGGX(s.dotNV, dotNL, s.roughness);
  vec3  F = F_Schlick(dotVH, s.F0);

  vec3 spec = (D * G * F) / max(4.0 * s.dotNV * dotNL, 0.001);
  vec3 kD = (vec3(1.0) - F) * (1.0 - s.metallic);
  vec3 diff = kD * s.albedo / PI;

  return (diff + spec) * dotNL;
}

vec3 evaluateIBL(Surface s) {
  vec3 ambient = vec3(0.0);
  
  if (env.hasIrradiance == 1 || env.hasPrefilter == 1) {
    if (env.hasIrradiance == 1) {
      vec3 kS = F_SchlickRoughness(s.dotNV, s.F0, s.roughness);
      vec3 kD = (vec3(1.0) - kS) * (1.0 - s.metallic);
      vec3 irradiance = texture(irradianceMap, directionToUV(s.N)).rgb;
      ambient += irradiance * s.albedo * kD;
    }

    if (env.hasPrefilter == 1) {
      float maxL = float(env.prefilterLayers - 1);
      float rawLayer = s.roughness * maxL;
      vec2  uv = directionToUV(s.R);
      
      vec3 spec0 = texture(prefilterMapArray, vec3(uv, floor(rawLayer))).rgb;
      vec3 spec1 = texture(prefilterMapArray, vec3(uv, min(ceil(rawLayer), maxL))).rgb;
      vec3 prefiltered = mix(spec0, spec1, fract(rawLayer));

      vec3 kS = F_SchlickRoughness(s.dotNV, s.F0, s.roughness);
      vec2 brdf = EnvBRDFApprox(s.roughness, s.dotNV);
      ambient += prefiltered * (kS * brdf.x + brdf.y);
    }

    return ambient * env.intensity;
  }
  
  return env.ambientColor * s.albedo * (1.0 - s.metallic);
}

void main() {
  Surface s;

  s.albedo = mat.baseColorFactor.rgb;
  if (mat.hasBaseColorMap == 1) s.albedo *= texture(baseColorMap, inUV).rgb;

  s.metallic = mat.metallicFactor;
  s.roughness = mat.roughnessFactor;
  if (mat.hasMetallicRoughnessMap == 1) {
    vec4 mr = texture(metallicRoughnessMap, inUV);
    s.metallic *= mr.b;
    s.roughness *= mr.g;
  }

  s.N = normalize(inNormal);
  mat3 TBN = (length(inTangent) > 0.1 && length(inBitangent) > 0.1) 
    ? mat3(normalize(inTangent), normalize(inBitangent), s.N) 
    : calculateTBN(inWorldPos, inUV, s.N);

  if (mat.hasNormalMap == 1) {
    s.N = normalize(TBN * (texture(normalMap, inUV).xyz * 2.0 - 1.0));
  }

  s.V = normalize(camera.position - inWorldPos);
  s.R = reflect(-s.V, s.N);
  s.dotNV = max(dot(s.N, s.V), 1e-4);
  
  float f0_raw = (mat.ior - 1.0) / (mat.ior + 1.0);
  s.F0 = mix(vec3(f0_raw * f0_raw), s.albedo, s.metallic);

  vec3 L = normalize(vec3(1.0, 1.0, 1.0));
  vec3 directResult = evaluateDirectPBR(s, L);

  vec3 indirectResult = evaluateIBL(s);

  vec3 coatDirect = vec3(0.0);
  vec3 coatIndirect = vec3(0.0);
  float coatF = 0.0;

  float coatFactor = mat.clearcoatFactor;
  if (mat.hasClearcoatMap == 1) coatFactor *= texture(clearcoatMap, inUV).r;

  if (coatFactor > 0.0) {
    vec3 coatN = (mat.hasClearcoatNormalMap == 1) 
      ? normalize(TBN * (texture(clearcoatNormalMap, inUV).xyz * 2.0 - 1.0)) 
      : normalize(inNormal);
    
    float dotNL_c = clamp01(dot(coatN, L));
    float dotNV_c = max(dot(coatN, s.V), 1e-4);
    vec3  H_c = normalize(s.V + L);

    float D_c = D_GGX(clamp01(dot(coatN, H_c)), mat.clearcoatRoughnessFactor);
    float G_c = G_SchlickGGX(dotNV_c, dotNL_c, mat.clearcoatRoughnessFactor);
    coatF = F_Schlick_Scalar(clamp01(dot(s.V, H_c)), 0.04) * coatFactor;
    coatDirect = (D_c * G_c * coatF) / max(4.0 * dotNV_c * dotNL_c, 0.001) * dotNL_c;

    if (env.hasPrefilter == 1) {
      vec3 coatR = reflect(-s.V, coatN);
      float maxL = float(env.prefilterLayers - 1);
      float coatRawL = mat.clearcoatRoughnessFactor * maxL;
      vec2  coatUV = directionToUV(coatR);
      
      vec3 c0 = texture(prefilterMapArray, vec3(coatUV, floor(coatRawL))).rgb;
      vec3 c1 = texture(prefilterMapArray, vec3(coatUV, min(ceil(coatRawL), maxL))).rgb;
      vec3 prefilterCoat = mix(c0, c1, fract(coatRawL));

      vec2 coatBRDF = EnvBRDFApprox(mat.clearcoatRoughnessFactor, dotNV_c);
      coatIndirect = prefilterCoat * (coatF * coatBRDF.x + coatBRDF.y) * env.intensity;
    }
  }

  vec3 baseLayer = directResult + indirectResult;
  vec3 finalColor = baseLayer * (1.0 - coatF) + coatDirect + coatIndirect;

  if (mat.hasEmissiveMap == 1) finalColor += mat.emissiveFactor * texture(emissiveMap, inUV).rgb;
  else finalColor += mat.emissiveFactor;

  finalColor = finalColor / (finalColor + vec3(1.0));

  outColor = vec4(finalColor, mat.baseColorFactor.a);
}
