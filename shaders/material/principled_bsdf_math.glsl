#ifndef MATERIAL_BSDF_MATH_GLSL
#define MATERIAL_BSDF_MATH_GLSL

const float PI = 3.14159265359;

float clamp01(float v) {
  return clamp(v, 0.0, 1.0);
}

mat3 calculateTBN(vec3 worldPos, vec2 uv, vec3 normal) {
  vec3 q1 = dFdx(worldPos);
  vec3 q2 = dFdy(worldPos);
  vec2 st1 = dFdx(uv);
  vec2 st2 = dFdy(uv);

  vec3 N = normalize(normal);
  float det = st1.x * st2.y - st2.x * st1.y;
  float invDet = (abs(det) > 1e-6) ? 1.0 / det : 1.0; 
  
  vec3 T = normalize((q1 * st2.y - q2 * st1.y) * invDet);
  T = normalize(T - dot(T, N) * N);
  vec3 B = cross(N, T);

  return mat3(T, B, N);
}

float D_GGX(float NdotH, float roughness) {
  float a = roughness * roughness;
  float a2 = a * a;
  float NdotH2 = NdotH * NdotH;
  float denom = (NdotH2 * (a2 - 1.0) + 1.0);
  return a2 / (PI * denom * denom + 1e-7);
}

float G_SchlickGGX(float NdotV, float NdotL, float roughness) {
  float r = (roughness + 1.0);
  float k = (r * r) / 8.0;
  float g1 = NdotV / (NdotV * (1.0 - k) + k);
  float g2 = NdotL / (NdotL * (1.0 - k) + k);
  return g1 * g2;
}

vec3 F_Schlick(float cosTheta, vec3 F0) {
  return F0 + (1.0 - F0) * pow(clamp01(1.0 - cosTheta), 5.0);
}

float F_Schlick_Scalar(float cosTheta, float F0) {
  return F0 + (1.0 - F0) * pow(clamp01(1.0 - cosTheta), 5.0);
}

vec3 F_SchlickRoughness(float cosTheta, vec3 F0, float roughness) {
  return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp01(1.0 - cosTheta), 5.0);
}

vec2 EnvBRDFApprox(float roughness, float NoV) {
  const vec4 c0 = vec4(-1.0, -0.0275, -0.572, 0.022);
  const vec4 c1 = vec4(1.0, 0.0425, 1.04, -0.04);
  vec4 r = roughness * c0 + c1;
  float a004 = min(r.x * r.x, exp2(-9.28 * NoV)) * r.x + r.y;
  return vec2(-1.04, 1.04) * a004 + r.zw;
}

vec2 directionToUV(vec3 v) {
  const vec2 invAtan = vec2(0.1591, 0.3183);
  vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
  uv *= invAtan;
  uv += 0.5;
  return uv;
}

float D_Charlie(float roughness, float dotNH) {
    float alpha = max(roughness, 0.000001);
    float invAlpha = 1.0 / alpha;
    float cos2h = dotNH * dotNH;
    float sin2h = max(1.0 - cos2h, 0.0078125); 
    return (2.0 + invAlpha) * pow(sin2h, invAlpha * 0.5) / (2.0 * PI);
}

float D_GGX_Anisotropic(float at, float ab, float dotToH, float dotBoH, float dotNH) {
    float a2 = at * ab;
    vec3 d = vec3(ab * dotToH, at * dotBoH, a2 * dotNH);
    float d2 = dot(d, d);
    float b2 = a2 / d2;
    return a2 * b2 * b2 * (1.0 / PI);
}

vec3 calculateVolumeAttenuation(float thickness, float attenuationDistance, vec3 attenuationColor) {
    if (attenuationDistance == 0.0 || thickness == 0.0) return vec3(1.0);
    vec3 attenuationCoefficient = -log(attenuationColor) / attenuationDistance;
    return exp(-attenuationCoefficient * thickness);
}

#endif // MATERIAL_BSDF_MATH_GLSL
