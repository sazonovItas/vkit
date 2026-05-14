#ifndef PRINCIPLED_BSDF_GLSL
#define PRINCIPLED_BSDF_GLSL

#include "principled_bsdf_math.glsl"

#define MAT_FEATURE_EMISSIVE      (1u << 0)
#define MAT_FEATURE_TRANSMISSION  (1u << 1)
#define MAT_FEATURE_CLEARCOAT     (1u << 2)
#define MAT_FEATURE_SHEEN         (1u << 3)
#define MAT_FEATURE_ANISOTROPY    (1u << 4)
#define MAT_FEATURE_IRIDESCENCE   (1u << 5)

struct PrincipledBSDFParams {
    vec4 baseColorFactor;
    vec3 emissiveFactor;
    float metallicFactor;

    float roughnessFactor;
    float occlusionStrength;
    float ior;
    uint featureMask;

    vec3 specularColorFactor;
    float specularFactor;

    float transmissionFactor;
    float thicknessFactor;
    float attenuationDistance;
    float padding1;

    vec3 attenuationColor;
    float padding2;

    vec3 sheenColorFactor;
    float sheenRoughnessFactor;

    float clearcoatFactor;
    float clearcoatRoughnessFactor;
    float anisotropyStrength;
    float iridescenceFactor;

    vec2 anisotropyRotation;
    float iridescenceIor;
    float iridescenceThicknessMin;

    float iridescenceThicknessMax;
    float padding3;
    float padding4;
    float padding5;
};

struct PrincipledBSDFTextures {
    int baseColorTexIdx;
    int normalTexIdx;
    int metallicRoughnessTexIdx;
    int emissiveTexIdx;

    int clearcoatTexIdx;
    int clearcoatNormalTexIdx;
    int occlusionTexIdx;
    int specularTexIdx;

    int specularColorTexIdx;
    int thicknessTexIdx;
    int sheenColorTexIdx;
    int sheenRoughnessTexIdx;

    int anisotropyTexIdx;
    int iridescenceTexIdx;
    int iridescenceThicknessTexIdx;
    int padding0;
};

struct PrincipledBSDFData {
    PrincipledBSDFParams params;
    PrincipledBSDFTextures textures;
};

struct Surface {
    vec3 pos, N, V, R, T, B, F0, sheenColor;
    vec4 albedo;
    float roughness, metallic, dotNV, ao, sheenRoughness, anisotropy, iridescence;
    vec2 uv;
};

struct BaseLighting {
    vec3 diffuse;
    vec3 specular;
};

Surface buildSurface(vec3 hitPos, vec3 viewDir, mat3 baseTBN, vec2 uv, PrincipledBSDFData mat) {
    Surface s;
    s.pos = hitPos;
    s.uv = uv;
    s.V = viewDir;

    s.T = baseTBN[0];
    s.B = baseTBN[1];
    s.N = baseTBN[2];

    s.albedo = mat.params.baseColorFactor;
    if (mat.textures.baseColorTexIdx >= 0) {
        s.albedo.rgb *= sampleTexture2DLinear(uint(mat.textures.baseColorTexIdx), s.uv).rgb;
    }

    s.metallic = mat.params.metallicFactor;
    s.roughness = mat.params.roughnessFactor;
    if (mat.textures.metallicRoughnessTexIdx >= 0) {
        vec4 mr = sampleTexture2DLinear(uint(mat.textures.metallicRoughnessTexIdx), s.uv);
        s.roughness *= mr.g;
        s.metallic *= mr.b;
    }

    s.ao = 1.0;
    if (mat.textures.occlusionTexIdx >= 0) {
        s.ao = mix(1.0, sampleTexture2DLinear(uint(mat.textures.occlusionTexIdx), s.uv).r, mat.params.occlusionStrength);
    }

    if (mat.textures.normalTexIdx >= 0) {
        vec3 normalSample = sampleTexture2DLinear(uint(mat.textures.normalTexIdx), s.uv).xyz * 2.0 - 1.0;
        s.N = normalize(baseTBN * normalSample);
    }

    s.R = reflect(-s.V, s.N);
    s.dotNV = max(dot(s.N, s.V), 1e-4);

    vec3 dielectricF0 = vec3(pow((mat.params.ior - 1.0) / (mat.params.ior + 1.0), 2.0));

    float specWeight = mat.params.specularFactor;
    if (mat.textures.specularTexIdx >= 0) specWeight *= sampleTexture2DLinear(uint(mat.textures.specularTexIdx), s.uv).a;

    vec3 specColor = mat.params.specularColorFactor;
    if (mat.textures.specularColorTexIdx >= 0) specColor *= sampleTexture2DLinear(uint(mat.textures.specularColorTexIdx), s.uv).rgb;

    s.F0 = mix(min(dielectricF0 * specColor * specWeight, vec3(1.0)), s.albedo.rgb, s.metallic);

    s.sheenColor = vec3(0.0);
    s.sheenRoughness = 0.0;
    if ((mat.params.featureMask & MAT_FEATURE_SHEEN) != 0u) {
        s.sheenColor = mat.params.sheenColorFactor;
        if (mat.textures.sheenColorTexIdx >= 0) s.sheenColor *= sampleTexture2DLinear(uint(mat.textures.sheenColorTexIdx), s.uv).rgb;

        s.sheenRoughness = mat.params.sheenRoughnessFactor;
        if (mat.textures.sheenRoughnessTexIdx >= 0) s.sheenRoughness *= sampleTexture2DLinear(uint(mat.textures.sheenRoughnessTexIdx), s.uv).a;
    }

    s.iridescence = 0.0;
    if ((mat.params.featureMask & MAT_FEATURE_IRIDESCENCE) != 0u) {
        s.iridescence = mat.params.iridescenceFactor;
        if (mat.textures.iridescenceTexIdx >= 0) s.iridescence *= sampleTexture2DLinear(uint(mat.textures.iridescenceTexIdx), s.uv).r;
    }

    s.anisotropy = 0.0;
    if ((mat.params.featureMask & MAT_FEATURE_ANISOTROPY) != 0u) {
        s.anisotropy = mat.params.anisotropyStrength;
        if (mat.textures.anisotropyTexIdx >= 0) {
            vec3 aniTex = sampleTexture2DLinear(uint(mat.textures.anisotropyTexIdx), s.uv).rgb * 2.0 - 1.0;
            s.anisotropy *= length(aniTex.xy);
        }
        s.T = normalize(baseTBN * vec3(mat.params.anisotropyRotation.x, mat.params.anisotropyRotation.y, 0.0));
        s.B = normalize(cross(s.N, s.T));
    }

    return s;
}

BaseLighting evaluateBaseLayer(Surface s, vec3 L, EnvironmentParams env) {
    BaseLighting result;
    vec3 currentF0 = mix(s.F0, vec3(1.0), s.iridescence * 0.5);
    vec3 H = normalize(s.V + L);
    float dotNL = clamp01(dot(s.N, L));
    float dotNH = clamp01(dot(s.N, H));
    float dotVH = clamp01(dot(s.V, H));

    float D = s.anisotropy > 0.0
        ? D_GGX_Anisotropic(max(s.roughness * (1.0 + s.anisotropy), 0.001), max(s.roughness * (1.0 - s.anisotropy), 0.001), dot(s.T, H), dot(s.B, H), dotNH) : D_GGX(dotNH, s.roughness);

    float G = G_SchlickGGX(s.dotNV, dotNL, s.roughness);
    vec3 F_dir = F_Schlick(dotVH, currentF0);

    vec3 specDirect = ((D * G * F_dir) / max(4.0 * s.dotNV * dotNL, 0.001)) * dotNL;

    vec3 diffDirect = ((vec3(1.0) - F_dir) * (1.0 - s.metallic) * s.albedo.rgb / PI) * dotNL;

    vec3 diffIndirect = vec3(0.0);
    vec3 specIndirect = vec3(0.0);
    vec3 F_ind = F_SchlickRoughness(s.dotNV, currentF0, s.roughness);

    // --- Environment IBL ---
    if ((env.features & ENV_FEATURE_USE_DIFFUSE) != 0u && env.irradianceTexIdx >= 0) {
        vec3 irradiance = sampleTexture2DLinear(uint(env.irradianceTexIdx), directionToUV(s.N)).rgb;
        diffIndirect = irradiance * s.albedo.rgb * ((vec3(1.0) - F_ind) * (1.0 - s.metallic));
    }

    if ((env.features & ENV_FEATURE_USE_SPECULAR) != 0u && env.prefilterTexIdx >= 0 && env.brdfLutTexIdx >= 0) {
        float rawL = s.roughness * float(env.prefilterNumLayers - 1);
        vec3 refDir = s.anisotropy > 0.0 ? normalize(mix(s.R, s.T, s.anisotropy * 0.5)) : s.R;

        vec2 refUV = directionToUV(refDir);
        vec3 prefFloor = sampleTexture2DArrayLinear(uint(env.prefilterTexIdx), vec3(refUV, floor(rawL))).rgb;
        vec3 prefCeil = sampleTexture2DArrayLinear(uint(env.prefilterTexIdx), vec3(refUV, ceil(rawL))).rgb;
        vec3 prefiltered = mix(prefFloor, prefCeil, fract(rawL));

        vec2 brdf = sampleTexture2DLinear(uint(env.brdfLutTexIdx), vec2(s.roughness, s.dotNV)).rg;
        specIndirect = prefiltered * (F_ind * brdf.x + brdf.y);
    }

    result.diffuse = diffDirect + (diffIndirect * env.intensity * s.ao);
    result.specular = specDirect + (specIndirect * env.intensity * s.ao);
    return result;
}

vec3 applyTransmissionLayer(Surface s, vec3 baseDiffuse, PrincipledBSDFData mat, EnvironmentParams env) {
    vec3 T = refract(-s.V, s.N, 1.0 / mat.params.ior);
    if (length(T) < 0.001) return baseDiffuse;

    vec3 refracted = vec3(1.0);

    if ((env.features & ENV_FEATURE_USE_SPECULAR) != 0u && env.prefilterTexIdx >= 0) {
        float rawL = s.roughness * float(env.prefilterNumLayers - 1);
        vec2 tUV = directionToUV(T);
        vec3 prefFloor = sampleTexture2DArrayLinear(uint(env.prefilterTexIdx), vec3(tUV, floor(rawL))).rgb;
        vec3 prefCeil = sampleTexture2DArrayLinear(uint(env.prefilterTexIdx), vec3(tUV, ceil(rawL))).rgb;
        refracted = mix(prefFloor, prefCeil, fract(rawL));
    }

    float thickness = mat.params.thicknessFactor;
    if (mat.textures.thicknessTexIdx >= 0) {
        thickness *= sampleTexture2DLinear(uint(mat.textures.thicknessTexIdx), s.uv).g;
    }

    vec3 atten = calculateVolumeAttenuation(thickness, mat.params.attenuationDistance, mat.params.attenuationColor);

    vec3 transLight = refracted * s.albedo.rgb * atten * env.intensity;
    return mix(baseDiffuse, transLight, mat.params.transmissionFactor * (1.0 - s.metallic));
}

vec3 applySheenLayer(Surface s, vec3 L, vec3 underlyingColor) {
    vec3 H = normalize(s.V + L);
    float dotNH = clamp01(dot(s.N, H));
    float dotNL = clamp01(dot(s.N, L));

    vec3 sheenBrdf = s.sheenColor * D_Charlie(s.sheenRoughness, dotNH) * G_Ashikhmin(dotNL, s.dotNV) * dotNL;

    // Energy conservation: sheen peaks at grazing (low NdotV), so the base layer
    // is attenuated proportionally there. Approximation avoids a precomputed sheen-E LUT.
    float maxSheen = max(s.sheenColor.r, max(s.sheenColor.g, s.sheenColor.b));
    float sheenScaling = 1.0 - maxSheen * (1.0 - s.dotNV * s.dotNV);
    return underlyingColor * clamp(sheenScaling, 0.0, 1.0) + sheenBrdf;
}

vec3 applyClearcoatLayer(Surface s, vec3 L, mat3 baseTBN, vec3 underlyingColor, PrincipledBSDFData mat, EnvironmentParams env) {
    float coatFactor = mat.params.clearcoatFactor;
    if (mat.textures.clearcoatTexIdx >= 0) {
        coatFactor *= sampleTexture2DLinear(uint(mat.textures.clearcoatTexIdx), s.uv).r;
    }

    vec3 coatN = s.N;
    if (mat.textures.clearcoatNormalTexIdx >= 0) {
        coatN = normalize(baseTBN * (sampleTexture2DLinear(uint(mat.textures.clearcoatNormalTexIdx), s.uv).xyz * 2.0 - 1.0));
    }

    float dotNV_c = max(dot(coatN, s.V), 1e-4);
    float dotNL_c = clamp01(dot(coatN, L));
    vec3 H_c = normalize(s.V + L);

    float D_c = D_GGX(clamp01(dot(coatN, H_c)), mat.params.clearcoatRoughnessFactor);
    float G_c = G_SchlickGGX(dotNV_c, dotNL_c, mat.params.clearcoatRoughnessFactor);
    vec3 coatF = F_Schlick(clamp01(dot(s.V, H_c)), vec3(0.04)) * coatFactor;

    vec3 coatDirect = ((D_c * G_c * coatF) / max(4.0 * dotNV_c * dotNL_c, 0.001)) * dotNL_c;
    vec3 coatIndirect = vec3(0.0);

    if ((env.features & ENV_FEATURE_USE_SPECULAR) != 0u && env.prefilterTexIdx >= 0 && env.brdfLutTexIdx >= 0) {
        float rawL = mat.params.clearcoatRoughnessFactor * float(env.prefilterNumLayers - 1);
        vec3 coatR = reflect(-s.V, coatN);
        vec2 coatUV = directionToUV(coatR);

        vec3 prefFloor = sampleTexture2DArrayLinear(uint(env.prefilterTexIdx), vec3(coatUV, floor(rawL))).rgb;
        vec3 prefCeil = sampleTexture2DArrayLinear(uint(env.prefilterTexIdx), vec3(coatUV, ceil(rawL))).rgb;
        vec3 prefCoat = mix(prefFloor, prefCeil, fract(rawL));

        vec2 brdf = sampleTexture2DLinear(uint(env.brdfLutTexIdx), vec2(mat.params.clearcoatRoughnessFactor, dotNV_c)).rg;
        coatIndirect = prefCoat * (coatF * brdf.x + coatFactor * brdf.y) * env.intensity;
    }

    return underlyingColor * (vec3(1.0) - coatF) + (coatDirect + coatIndirect);
}

vec3 evaluateUberLighting(Surface s, vec3 L, mat3 baseTBN, PrincipledBSDFData mat, EnvironmentParams env) {
    BaseLighting base = evaluateBaseLayer(s, L, env);
    vec3 finalColor = base.diffuse;

    if ((mat.params.featureMask & MAT_FEATURE_TRANSMISSION) != 0u) {
        finalColor = applyTransmissionLayer(s, finalColor, mat, env);
    }

    finalColor += base.specular;

    if ((mat.params.featureMask & MAT_FEATURE_SHEEN) != 0u) {
        finalColor = applySheenLayer(s, L, finalColor);
    }

    if ((mat.params.featureMask & MAT_FEATURE_CLEARCOAT) != 0u) {
        finalColor = applyClearcoatLayer(s, L, baseTBN, finalColor, mat, env);
    }

    if ((mat.params.featureMask & MAT_FEATURE_EMISSIVE) != 0u) {
        vec3 emissive = mat.params.emissiveFactor;
        if (mat.textures.emissiveTexIdx >= 0) {
            emissive *= sampleTexture2DLinear(uint(mat.textures.emissiveTexIdx), s.uv).rgb;
        }
        finalColor += emissive;
    }

    return finalColor;
}

#endif // MATERIAL_PRINCIPLED_BSDF_GLSL
