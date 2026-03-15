#ifndef MATERIAL_GLSL
#define MATERIAL_GLSL

  struct Material {
    vec4 baseColorFactor;
    vec4 emissiveFactor;

    float metallicFactor;
    float roughnessFactor;
    float alphaMaskCutoff;
    float emissiveStrength;

    int baseColorTextureIdx;
    int metallicRoughnessTextureIdx;
    int normalTextureIdx;
    int occlusionTextureIdx;

    int emissiveTextureIdx;
    int pad0;
    int pad1;
    float dissolveStrength;
  };

#endif // MATERIAL_GLSL
