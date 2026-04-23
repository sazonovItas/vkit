#ifndef BSDF_MATH_GLSL
#define BSDF_MATH_GLSL

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

#endif // BSDF_MATH_GLSL
