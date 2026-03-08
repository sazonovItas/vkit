#ifndef MATERIAL_GLSL
#define MATERIAL_GLSL

struct Material {
  vec4 baseColorFactor;
  vec4 emissiveFactor;
  vec4 diffuseFactor;
  vec4 specularFactor;

  int baseColorTextureIdx;
  int metallicRoughnessTextureIdx;
  int normalTextureIdx;
  int occlusionTextureIdx;

  int emissiveTextureIdx;
  int _padding0[3];

  float metallicFactor;
  float roughnessFactor;
  float alphaMask;
  float alphaMaskCutoff;

  float emissiveStrength;
  float _padding1[3];
};

#endif // MATERIAL_GLSL
