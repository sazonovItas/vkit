#ifndef MATERIAL_BSDF_LAYOUTS
#define MATERIAL_BSDF_LAYOUTS

#ifndef MATERIAL_BSDF_LAYOUTS_SET
#define MATERIAL_BSDF_LAYOUTS_SET 3
#endif

layout(set = MATERIAL_BSDF_LAYOUTS_SET, binding = 0) uniform Material {
    vec4  baseColorFactor;
    vec3  emissiveFactor;
    float metallicFactor;
    float roughnessFactor;

    float ior;
    float clearcoatFactor;
    float clearcoatRoughnessFactor;
    float transmissionFactor;

    float occlusionStrength;
    float specularFactor;
    vec3  specularColorFactor;
    
    float thicknessFactor;
    vec3  attenuationColor;
    float attenuationDistance;

    vec3  sheenColorFactor;
    float sheenRoughnessFactor;

    float anisotropyStrength;
    vec2  anisotropyRotation;

    float iridescenceFactor;
    float iridescenceIor;
    float iridescenceThicknessMin;
    float iridescenceThicknessMax;

    uint  hasBaseColorMap;
    uint  hasNormalMap;
    uint  hasMetallicRoughnessMap;
    uint  hasEmissiveMap;
    uint  hasClearcoatMap;
    uint  hasClearcoatNormalMap;
    uint  hasOcclusionMap;
    uint  hasSpecularMap;
    uint  hasSpecularColorMap;
    uint  hasThicknessMap;
    uint  hasSheenColorMap;
    uint  hasSheenRoughnessMap;
    uint  hasAnisotropyMap;
    uint  hasIridescenceMap;
    uint  hasIridescenceThicknessMap;
} mat;

layout(set = MATERIAL_BSDF_LAYOUTS_SET, binding = 1) uniform sampler2D baseColorMap;
layout(set = MATERIAL_BSDF_LAYOUTS_SET, binding = 2) uniform sampler2D normalMap;
layout(set = MATERIAL_BSDF_LAYOUTS_SET, binding = 3) uniform sampler2D metallicRoughnessMap;
layout(set = MATERIAL_BSDF_LAYOUTS_SET, binding = 4) uniform sampler2D emissiveMap;
layout(set = MATERIAL_BSDF_LAYOUTS_SET, binding = 5) uniform sampler2D clearcoatMap;
layout(set = MATERIAL_BSDF_LAYOUTS_SET, binding = 6) uniform sampler2D clearcoatNormalMap;
layout(set = MATERIAL_BSDF_LAYOUTS_SET, binding = 7) uniform sampler2D occlusionMap;
layout(set = MATERIAL_BSDF_LAYOUTS_SET, binding = 8) uniform sampler2D specularMap;
layout(set = MATERIAL_BSDF_LAYOUTS_SET, binding = 9) uniform sampler2D specularColorMap;
layout(set = MATERIAL_BSDF_LAYOUTS_SET, binding = 10) uniform sampler2D thicknessMap;
layout(set = MATERIAL_BSDF_LAYOUTS_SET, binding = 11) uniform sampler2D sheenColorMap;
layout(set = MATERIAL_BSDF_LAYOUTS_SET, binding = 12) uniform sampler2D sheenRoughnessMap;
layout(set = MATERIAL_BSDF_LAYOUTS_SET, binding = 13) uniform sampler2D anisotropyMap;
layout(set = MATERIAL_BSDF_LAYOUTS_SET, binding = 14) uniform sampler2D iridescenceMap;
layout(set = MATERIAL_BSDF_LAYOUTS_SET, binding = 15) uniform sampler2D iridescenceThicknessMap;

#endif // MATERIAL_BSDF_LAYOUTS_SETS
