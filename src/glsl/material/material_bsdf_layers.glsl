#ifndef MATERIAL_BSDF_LAYERS
#define MATERIAL_BSDF_LAYERS

#include "material_bsdf_math.glsl"

struct Surface {
  vec3  pos, N, V, R, T, B, albedo, F0, sheenColor;
  float roughness, metallic, dotNV, ao, sheenRoughness, anisotropy, iridescence;
  vec2  uv;
};

struct BaseLighting {
  vec3 diffuse;
  vec3 specular;
};

Surface buildSurface(vec3 hitPos, vec3 geomNormal, vec2 uv, vec3 viewDir, mat3 baseTBN) {
  Surface s;
  s.pos = hitPos; s.uv = uv; s.V = viewDir;

  s.albedo = mat.baseColorFactor.rgb;
  if (mat.hasBaseColorMap == 1) s.albedo *= texture(baseColorMap, s.uv).rgb;

  s.metallic = mat.metallicFactor; 
  s.roughness = mat.roughnessFactor;
  if (mat.hasMetallicRoughnessMap == 1) {
    vec4 mr = texture(metallicRoughnessMap, s.uv);
    s.metallic *= mr.b; 
    s.roughness *= mr.g;
  }

  s.ao = mat.hasOcclusionMap == 1 ? mix(1.0, texture(occlusionMap, s.uv).r, mat.occlusionStrength) : 1.0;

  s.N = geomNormal;
  if (mat.hasNormalMap == 1) {
    s.N = normalize(baseTBN * (texture(normalMap, s.uv).xyz * 2.0 - 1.0));
  }

  s.R = reflect(-s.V, s.N);
  s.dotNV = max(dot(s.N, s.V), 1e-4);

  vec3 dielectricF0 = vec3(pow((mat.ior - 1.0) / (mat.ior + 1.0), 2.0));
  float specWeight = mat.hasSpecularMap == 1 ? mat.specularFactor * texture(specularMap, s.uv).a : mat.specularFactor;
  vec3 specColor = mat.hasSpecularColorMap == 1 ? mat.specularColorFactor * texture(specularColorMap, s.uv).rgb : mat.specularColorFactor;
  s.F0 = mix(min(dielectricF0 * specColor * specWeight, vec3(1.0)), s.albedo, s.metallic);

  s.sheenColor = mat.hasSheenColorMap == 1 ? mat.sheenColorFactor * texture(sheenColorMap, s.uv).rgb : mat.sheenColorFactor;
  s.sheenRoughness = mat.hasSheenRoughnessMap == 1 ? mat.sheenRoughnessFactor * texture(sheenRoughnessMap, s.uv).a : mat.sheenRoughnessFactor;
  s.iridescence = mat.hasIridescenceMap == 1 ? mat.iridescenceFactor * texture(iridescenceMap, s.uv).r : mat.iridescenceFactor;
  
  s.anisotropy = mat.anisotropyStrength;
  if (mat.hasAnisotropyMap == 1) {
    vec3 aniTex = texture(anisotropyMap, s.uv).rgb * 2.0 - 1.0;
    s.anisotropy *= length(aniTex.xy);
    s.T = normalize(baseTBN * vec3(mat.anisotropyRotation.x, mat.anisotropyRotation.y, 0.0));
    s.B = normalize(cross(s.N, s.T));
  }

  return s;
}

BaseLighting evaluateBaseLayer(Surface s, vec3 L) {
  BaseLighting result;
  vec3 currentF0 = mix(s.F0, vec3(1.0), s.iridescence * 0.5); 
  vec3 H = normalize(s.V + L);
  float dotNL = clamp01(dot(s.N, L));
  float dotNH = clamp01(dot(s.N, H));
  float dotVH = clamp01(dot(s.V, H));

  float D = s.anisotropy > 0.0 
    ? D_GGX_Anisotropic(max(s.roughness * (1.0 + s.anisotropy), 0.001), max(s.roughness * (1.0 - s.anisotropy), 0.001), dot(s.T, H), dot(s.B, H), dotNH)
    : D_GGX(dotNH, s.roughness);
  
  float G = G_SchlickGGX(s.dotNV, dotNL, s.roughness);
  vec3 F_dir = F_Schlick(dotVH, currentF0);

  vec3 specDirect = ((D * G * F_dir) / max(4.0 * s.dotNV * dotNL, 0.001)) * dotNL;
  vec3 diffDirect = ((vec3(1.0) - F_dir) * (1.0 - s.metallic) * s.albedo / PI) * dotNL;

  vec3 diffIndirect = vec3(0.0);
  vec3 specIndirect = vec3(0.0);
  vec3 F_ind = F_SchlickRoughness(s.dotNV, currentF0, s.roughness);

  if (env.hasIrradiance == 1) {
    diffIndirect = texture(irradianceMap, directionToUV(s.N)).rgb * s.albedo * ((vec3(1.0) - F_ind) * (1.0 - s.metallic));
  }

  if (env.hasPrefilter == 1) {
    float rawL = s.roughness * float(env.prefilterLayers - 1);
    vec3 refDir = s.anisotropy > 0.0 ? normalize(mix(s.R, s.T, s.anisotropy * 0.5)) : s.R;
    
    vec3 prefiltered = mix(texture(prefilterMapArray, vec3(directionToUV(refDir), floor(rawL))).rgb, 
                           texture(prefilterMapArray, vec3(directionToUV(refDir), ceil(rawL))).rgb, fract(rawL));

    vec2 brdf = EnvBRDFApprox(s.roughness, s.dotNV);
    specIndirect = prefiltered * (F_ind * brdf.x + brdf.y);
  }

  result.diffuse = diffDirect + (diffIndirect * env.intensity * s.ao);
  result.specular = specDirect + (specIndirect * env.intensity * s.ao);
  return result;
}

vec3 applyTransmissionLayer(Surface s, vec3 baseDiffuse) {
  if (mat.transmissionFactor <= 0.0) return baseDiffuse;

  vec3 T = refract(-s.V, s.N, 1.0 / mat.ior);
  if (length(T) < 0.001) return baseDiffuse; 

  vec3 refracted = env.ambientColor;
  if (env.hasPrefilter == 1) {
    float rawL = s.roughness * float(env.prefilterLayers - 1);
    refracted = mix(texture(prefilterMapArray, vec3(directionToUV(T), floor(rawL))).rgb, 
                    texture(prefilterMapArray, vec3(directionToUV(T), ceil(rawL))).rgb, fract(rawL));
  }

  float thickness = mat.hasThicknessMap == 1 ? mat.thicknessFactor * texture(thicknessMap, s.uv).g : mat.thicknessFactor;
  vec3 atten = calculateVolumeAttenuation(thickness, mat.attenuationDistance, mat.attenuationColor);
  
  vec3 transLight = refracted * s.albedo * atten * env.intensity;
  return mix(baseDiffuse, transLight, mat.transmissionFactor * (1.0 - s.metallic));
}

vec3 applySheenLayer(Surface s, vec3 L, vec3 underlyingColor) {
  if (length(s.sheenColor) <= 0.0) return underlyingColor;
  vec3 H = normalize(s.V + L);
  return underlyingColor + (s.sheenColor * D_Charlie(s.sheenRoughness, clamp01(dot(s.N, H))) * G_SchlickGGX(s.dotNV, clamp01(dot(s.N, L)), s.sheenRoughness) * clamp01(dot(s.N, L)));
}

vec3 applyClearcoatLayer(Surface s, vec3 L, mat3 baseTBN, vec3 underlyingColor) {
  float coatFactor = mat.hasClearcoatMap == 1 ? mat.clearcoatFactor * texture(clearcoatMap, s.uv).r : mat.clearcoatFactor;
  if (coatFactor <= 0.0) return underlyingColor;

  vec3 coatN = mat.hasClearcoatNormalMap == 1 ? normalize(baseTBN * (texture(clearcoatNormalMap, s.uv).xyz * 2.0 - 1.0)) : s.N;
  float dotNV_c = max(dot(coatN, s.V), 1e-4);
  float dotNL_c = clamp01(dot(coatN, L));
  vec3 H_c = normalize(s.V + L);
  
  float D_c = D_GGX(clamp01(dot(coatN, H_c)), mat.clearcoatRoughnessFactor);
  float G_c = G_SchlickGGX(dotNV_c, dotNL_c, mat.clearcoatRoughnessFactor);
  vec3 coatF = F_Schlick(clamp01(dot(s.V, H_c)), vec3(0.04)) * coatFactor;

  vec3 coatDirect = (D_c * G_c * coatF) / max(4.0 * dotNV_c * dotNL_c, 0.001) * dotNL_c;
  vec3 coatIndirect = vec3(0.0);

  if (env.hasPrefilter == 1) {
    float rawL = mat.clearcoatRoughnessFactor * float(env.prefilterLayers - 1);
    vec3 coatR = reflect(-s.V, coatN);
    vec3 prefCoat = mix(texture(prefilterMapArray, vec3(directionToUV(coatR), floor(rawL))).rgb, 
                        texture(prefilterMapArray, vec3(directionToUV(coatR), ceil(rawL))).rgb, fract(rawL));
    vec2 brdf = EnvBRDFApprox(mat.clearcoatRoughnessFactor, dotNV_c);
    coatIndirect = prefCoat * (coatF * brdf.x + brdf.y) * env.intensity;
  }

  return underlyingColor * (vec3(1.0) - coatF) + (coatDirect + coatIndirect);
}

vec3 evaluateUberLighting(Surface s, vec3 L, mat3 baseTBN) {
  BaseLighting base = evaluateBaseLayer(s, L);
  vec3 diffuseLayer = applyTransmissionLayer(s, base.diffuse);
  vec3 finalColor = applySheenLayer(s, L, diffuseLayer + base.specular);
  finalColor = applyClearcoatLayer(s, L, baseTBN, finalColor);

  vec3 emissive = mat.hasEmissiveMap == 1 ? mat.emissiveFactor * texture(emissiveMap, s.uv).rgb : mat.emissiveFactor;
  return finalColor + emissive;
}

#endif // MATERIAL_BSDF_LAYERS
