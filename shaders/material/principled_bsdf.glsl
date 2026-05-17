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

// ── Direct-light BRDF (no IBL). Result is multiplied by lightColor externally.
BaseLighting evaluateBaseLayerDirect(Surface s, vec3 L, float shadowFactor) {
    BaseLighting result;
    vec3 currentF0 = mix(s.F0, vec3(1.0), s.iridescence * 0.5);
    vec3 H = normalize(s.V + L);
    float dotNL = clamp01(dot(s.N, L));
    float dotNH = clamp01(dot(s.N, H));
    float dotVH = clamp01(dot(s.V, H));

    float D = s.anisotropy > 0.0
        ? D_GGX_Anisotropic(max(s.roughness*(1.0+s.anisotropy),0.001), max(s.roughness*(1.0-s.anisotropy),0.001), dot(s.T,H), dot(s.B,H), dotNH)
        : D_GGX(dotNH, s.roughness);

    float G    = G_SchlickGGX(s.dotNV, dotNL, s.roughness);
    vec3 F_dir = F_Schlick(dotVH, currentF0);

    result.specular = ((D * G * F_dir) / max(4.0 * s.dotNV * dotNL, 0.001)) * dotNL * shadowFactor;
    result.diffuse  = ((vec3(1.0) - F_dir) * (1.0 - s.metallic) * s.albedo.rgb / PI) * dotNL * shadowFactor;
    return result;
}

// ── IBL (ambient) contribution only.
BaseLighting evaluateBaseLayerIBL(Surface s, EnvironmentParams env) {
    BaseLighting result;
    result.diffuse  = vec3(0.0);
    result.specular = vec3(0.0);
    vec3 currentF0 = mix(s.F0, vec3(1.0), s.iridescence * 0.5);
    vec3 F_ind = F_SchlickRoughness(s.dotNV, currentF0, s.roughness);

    if ((env.features & ENV_FEATURE_USE_DIFFUSE) != 0u && env.irradianceTexIdx >= 0) {
        vec3 irradiance = sampleTexture2DLinear(uint(env.irradianceTexIdx), directionToUV(s.N)).rgb;
        result.diffuse = irradiance * s.albedo.rgb * ((vec3(1.0) - F_ind) * (1.0 - s.metallic)) * env.intensity * s.ao;
    }

    if ((env.features & ENV_FEATURE_USE_SPECULAR) != 0u && env.prefilterTexIdx >= 0 && env.brdfLutTexIdx >= 0) {
        float rawL  = s.roughness * float(env.prefilterNumLayers - 1);
        vec3 refDir = s.anisotropy > 0.0 ? normalize(mix(s.R, s.T, s.anisotropy * 0.5)) : s.R;
        vec2 refUV  = directionToUV(refDir);
        vec3 prefFloor = sampleTexture2DArrayLinear(uint(env.prefilterTexIdx), vec3(refUV, floor(rawL))).rgb;
        vec3 prefCeil  = sampleTexture2DArrayLinear(uint(env.prefilterTexIdx), vec3(refUV, ceil(rawL))).rgb;
        vec3 prefiltered = mix(prefFloor, prefCeil, fract(rawL));
        vec2 brdf = sampleTexture2DLinear(uint(env.brdfLutTexIdx), vec2(s.dotNV, s.roughness)).rg;
        result.specular = prefiltered * (F_ind * brdf.x + brdf.y) * env.intensity * s.ao;
    }

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
        vec3 prefCeil  = sampleTexture2DArrayLinear(uint(env.prefilterTexIdx), vec3(tUV, ceil(rawL))).rgb;
        refracted = mix(prefFloor, prefCeil, fract(rawL));
    }

    float thickness = mat.params.thicknessFactor;
    if (mat.textures.thicknessTexIdx >= 0)
        thickness *= sampleTexture2DLinear(uint(mat.textures.thicknessTexIdx), s.uv).g;

    vec3 atten = calculateVolumeAttenuation(thickness, mat.params.attenuationDistance, mat.params.attenuationColor);
    vec3 transLight = refracted * s.albedo.rgb * atten * env.intensity;
    return mix(baseDiffuse, transLight, mat.params.transmissionFactor * (1.0 - s.metallic));
}

vec3 applySheenLayer(Surface s, vec3 L, vec3 underlyingColor) {
    vec3 H = normalize(s.V + L);
    float dotNH = clamp01(dot(s.N, H));
    float dotNL = clamp01(dot(s.N, L));
    vec3 sheenBrdf = s.sheenColor * D_Charlie(s.sheenRoughness, dotNH) * G_Ashikhmin(dotNL, s.dotNV) * dotNL;
    float maxSheen = max(s.sheenColor.r, max(s.sheenColor.g, s.sheenColor.b));
    float sheenScaling = 1.0 - maxSheen * (1.0 - s.dotNV * s.dotNV);
    return underlyingColor * clamp(sheenScaling, 0.0, 1.0) + sheenBrdf;
}

// Per-light clearcoat (direct specular + energy conservation attenuation).
vec3 applyClearcoatDirect(Surface s, vec3 L, mat3 baseTBN,
                          vec3 underlyingColor, PrincipledBSDFData mat, float shadowFactor) {
    float coatFactor = mat.params.clearcoatFactor;
    if (mat.textures.clearcoatTexIdx >= 0)
        coatFactor *= sampleTexture2DLinear(uint(mat.textures.clearcoatTexIdx), s.uv).r;

    vec3 coatN = s.N;
    if (mat.textures.clearcoatNormalTexIdx >= 0)
        coatN = normalize(baseTBN * (sampleTexture2DLinear(uint(mat.textures.clearcoatNormalTexIdx), s.uv).xyz * 2.0 - 1.0));

    float dotNV_c = max(dot(coatN, s.V), 1e-4);
    float dotNL_c = clamp01(dot(coatN, L));
    vec3  H_c     = normalize(s.V + L);
    float D_c     = D_GGX(clamp01(dot(coatN, H_c)), mat.params.clearcoatRoughnessFactor);
    float G_c     = G_SchlickGGX(dotNV_c, dotNL_c, mat.params.clearcoatRoughnessFactor);
    vec3  coatF   = F_Schlick(clamp01(dot(s.V, H_c)), vec3(0.04)) * coatFactor;
    vec3  coatDirect = ((D_c * G_c * coatF) / max(4.0 * dotNV_c * dotNL_c, 0.001)) * dotNL_c * shadowFactor;

    return underlyingColor * (vec3(1.0) - coatF) + coatDirect;
}

// IBL clearcoat: adds indirect specular and attenuates underlying via F_ind.
vec3 applyClearcoatIBL(Surface s, mat3 baseTBN, vec3 underlyingColor,
                        PrincipledBSDFData mat, EnvironmentParams env) {
    float coatFactor = mat.params.clearcoatFactor;
    if (mat.textures.clearcoatTexIdx >= 0)
        coatFactor *= sampleTexture2DLinear(uint(mat.textures.clearcoatTexIdx), s.uv).r;

    vec3 coatN = s.N;
    if (mat.textures.clearcoatNormalTexIdx >= 0)
        coatN = normalize(baseTBN * (sampleTexture2DLinear(uint(mat.textures.clearcoatNormalTexIdx), s.uv).xyz * 2.0 - 1.0));

    float dotNV_c = max(dot(coatN, s.V), 1e-4);
    vec3 coatF_ind = F_SchlickRoughness(dotNV_c, vec3(0.04), mat.params.clearcoatRoughnessFactor) * coatFactor;

    vec3 coatIndirect = vec3(0.0);
    if ((env.features & ENV_FEATURE_USE_SPECULAR) != 0u && env.prefilterTexIdx >= 0 && env.brdfLutTexIdx >= 0) {
        float rawL = mat.params.clearcoatRoughnessFactor * float(env.prefilterNumLayers - 1);
        vec3 coatR  = reflect(-s.V, coatN);
        vec2 coatUV = directionToUV(coatR);
        vec3 prefFloor = sampleTexture2DArrayLinear(uint(env.prefilterTexIdx), vec3(coatUV, floor(rawL))).rgb;
        vec3 prefCeil  = sampleTexture2DArrayLinear(uint(env.prefilterTexIdx), vec3(coatUV, ceil(rawL))).rgb;
        vec3 prefCoat  = mix(prefFloor, prefCeil, fract(rawL));
        vec2 brdf = sampleTexture2DLinear(uint(env.brdfLutTexIdx), vec2(dotNV_c, mat.params.clearcoatRoughnessFactor)).rg;
        coatIndirect = prefCoat * (coatF_ind * brdf.x + coatFactor * brdf.y) * env.intensity;
    }

    return underlyingColor * (vec3(1.0) - coatF_ind) + coatIndirect;
}

// ── IBL pass (call once per surface).
vec3 evaluateUberLightingIBL(Surface s, mat3 baseTBN, PrincipledBSDFData mat, EnvironmentParams env) {
    BaseLighting indirect = evaluateBaseLayerIBL(s, env);
    vec3 finalColor = indirect.diffuse;

    if ((mat.params.featureMask & MAT_FEATURE_TRANSMISSION) != 0u)
        finalColor = applyTransmissionLayer(s, finalColor, mat, env);

    finalColor += indirect.specular;

    if ((mat.params.featureMask & MAT_FEATURE_CLEARCOAT) != 0u)
        finalColor = applyClearcoatIBL(s, baseTBN, finalColor, mat, env);

    if ((mat.params.featureMask & MAT_FEATURE_EMISSIVE) != 0u) {
        vec3 emissive = mat.params.emissiveFactor;
        if (mat.textures.emissiveTexIdx >= 0)
            emissive *= sampleTexture2DLinear(uint(mat.textures.emissiveTexIdx), s.uv).rgb;
        finalColor += emissive;
    }

    return finalColor;
}

// ── Per-pixel PBSDF blend ─────────────────────────────────────────────────────
// Returns a blended PrincipledBSDFData where:
//   - All scalar params are linearly mixed (factor 0→a, 1→b).
//   - Multiplicative texture maps (baseColor, metallicRoughness, emissive,
//     specular, sheenColor/Roughness, clearcoat, iridescence) are sampled from
//     both materials, linearly blended, and baked into the params (texIdx=-1).
//   - Normal and other directional maps use threshold selection (< 0.5 → A).
PrincipledBSDFData sampleAndBlendPbsdf(PrincipledBSDFData a, PrincipledBSDFData b, float t, vec2 uv) {
    PrincipledBSDFData r;

    // --- Lerp all scalar params ---
    r.params.baseColorFactor          = mix(a.params.baseColorFactor,          b.params.baseColorFactor,          t);
    r.params.emissiveFactor           = mix(a.params.emissiveFactor,           b.params.emissiveFactor,           t);
    r.params.metallicFactor           = mix(a.params.metallicFactor,           b.params.metallicFactor,           t);
    r.params.roughnessFactor          = mix(a.params.roughnessFactor,          b.params.roughnessFactor,          t);
    r.params.occlusionStrength        = mix(a.params.occlusionStrength,        b.params.occlusionStrength,        t);
    r.params.ior                      = mix(a.params.ior,                      b.params.ior,                      t);
    r.params.specularColorFactor      = mix(a.params.specularColorFactor,      b.params.specularColorFactor,      t);
    r.params.specularFactor           = mix(a.params.specularFactor,           b.params.specularFactor,           t);
    r.params.transmissionFactor       = mix(a.params.transmissionFactor,       b.params.transmissionFactor,       t);
    r.params.thicknessFactor          = mix(a.params.thicknessFactor,          b.params.thicknessFactor,          t);
    r.params.attenuationDistance      = mix(a.params.attenuationDistance,      b.params.attenuationDistance,      t);
    r.params.attenuationColor         = mix(a.params.attenuationColor,         b.params.attenuationColor,         t);
    r.params.sheenColorFactor         = mix(a.params.sheenColorFactor,         b.params.sheenColorFactor,         t);
    r.params.sheenRoughnessFactor     = mix(a.params.sheenRoughnessFactor,     b.params.sheenRoughnessFactor,     t);
    r.params.clearcoatFactor          = mix(a.params.clearcoatFactor,          b.params.clearcoatFactor,          t);
    r.params.clearcoatRoughnessFactor = mix(a.params.clearcoatRoughnessFactor, b.params.clearcoatRoughnessFactor, t);
    r.params.anisotropyStrength       = mix(a.params.anisotropyStrength,       b.params.anisotropyStrength,       t);
    r.params.anisotropyRotation       = mix(a.params.anisotropyRotation,       b.params.anisotropyRotation,       t);
    r.params.iridescenceFactor        = mix(a.params.iridescenceFactor,        b.params.iridescenceFactor,        t);
    r.params.iridescenceIor           = mix(a.params.iridescenceIor,           b.params.iridescenceIor,           t);
    r.params.iridescenceThicknessMin  = mix(a.params.iridescenceThicknessMin,  b.params.iridescenceThicknessMin,  t);
    r.params.iridescenceThicknessMax  = mix(a.params.iridescenceThicknessMax,  b.params.iridescenceThicknessMax,  t);
    r.params.featureMask = a.params.featureMask | b.params.featureMask;
    r.params.padding1 = 0.0; r.params.padding2 = 0.0;
    r.params.padding3 = 0.0; r.params.padding4 = 0.0; r.params.padding5 = 0.0;

    // --- Base color: sample both, lerp, bake ---
    vec4 bcA = vec4(1.0), bcB = vec4(1.0);
    if (a.textures.baseColorTexIdx >= 0) bcA = sampleTexture2DLinear(uint(a.textures.baseColorTexIdx), uv);
    if (b.textures.baseColorTexIdx >= 0) bcB = sampleTexture2DLinear(uint(b.textures.baseColorTexIdx), uv);
    r.params.baseColorFactor *= mix(bcA, bcB, t);
    r.textures.baseColorTexIdx = -1;

    // --- Metallic/Roughness: sample both .gb channels, lerp, bake ---
    float mrRoughA = 1.0, mrMetalA = 1.0, mrRoughB = 1.0, mrMetalB = 1.0;
    if (a.textures.metallicRoughnessTexIdx >= 0) {
        vec4 mr = sampleTexture2DLinear(uint(a.textures.metallicRoughnessTexIdx), uv);
        mrRoughA = mr.g; mrMetalA = mr.b;
    }
    if (b.textures.metallicRoughnessTexIdx >= 0) {
        vec4 mr = sampleTexture2DLinear(uint(b.textures.metallicRoughnessTexIdx), uv);
        mrRoughB = mr.g; mrMetalB = mr.b;
    }
    r.params.roughnessFactor *= mix(mrRoughA, mrRoughB, t);
    r.params.metallicFactor  *= mix(mrMetalA, mrMetalB, t);
    r.textures.metallicRoughnessTexIdx = -1;

    // --- Emissive ---
    vec3 emA = vec3(1.0), emB = vec3(1.0);
    if (a.textures.emissiveTexIdx >= 0) emA = sampleTexture2DLinear(uint(a.textures.emissiveTexIdx), uv).rgb;
    if (b.textures.emissiveTexIdx >= 0) emB = sampleTexture2DLinear(uint(b.textures.emissiveTexIdx), uv).rgb;
    r.params.emissiveFactor *= mix(emA, emB, t);
    r.textures.emissiveTexIdx = -1;

    // --- Specular (alpha channel) ---
    float spA = 1.0, spB = 1.0;
    if (a.textures.specularTexIdx >= 0) spA = sampleTexture2DLinear(uint(a.textures.specularTexIdx), uv).a;
    if (b.textures.specularTexIdx >= 0) spB = sampleTexture2DLinear(uint(b.textures.specularTexIdx), uv).a;
    r.params.specularFactor *= mix(spA, spB, t);
    r.textures.specularTexIdx = -1;

    // --- Specular color (rgb) ---
    vec3 scA = vec3(1.0), scB = vec3(1.0);
    if (a.textures.specularColorTexIdx >= 0) scA = sampleTexture2DLinear(uint(a.textures.specularColorTexIdx), uv).rgb;
    if (b.textures.specularColorTexIdx >= 0) scB = sampleTexture2DLinear(uint(b.textures.specularColorTexIdx), uv).rgb;
    r.params.specularColorFactor *= mix(scA, scB, t);
    r.textures.specularColorTexIdx = -1;

    // --- Clearcoat (r channel) ---
    float ccA = 1.0, ccB = 1.0;
    if (a.textures.clearcoatTexIdx >= 0) ccA = sampleTexture2DLinear(uint(a.textures.clearcoatTexIdx), uv).r;
    if (b.textures.clearcoatTexIdx >= 0) ccB = sampleTexture2DLinear(uint(b.textures.clearcoatTexIdx), uv).r;
    r.params.clearcoatFactor *= mix(ccA, ccB, t);
    r.textures.clearcoatTexIdx = -1;

    // --- Sheen color ---
    vec3 shcA = vec3(1.0), shcB = vec3(1.0);
    if (a.textures.sheenColorTexIdx >= 0) shcA = sampleTexture2DLinear(uint(a.textures.sheenColorTexIdx), uv).rgb;
    if (b.textures.sheenColorTexIdx >= 0) shcB = sampleTexture2DLinear(uint(b.textures.sheenColorTexIdx), uv).rgb;
    r.params.sheenColorFactor *= mix(shcA, shcB, t);
    r.textures.sheenColorTexIdx = -1;

    // --- Sheen roughness (alpha channel) ---
    float shrA = 1.0, shrB = 1.0;
    if (a.textures.sheenRoughnessTexIdx >= 0) shrA = sampleTexture2DLinear(uint(a.textures.sheenRoughnessTexIdx), uv).a;
    if (b.textures.sheenRoughnessTexIdx >= 0) shrB = sampleTexture2DLinear(uint(b.textures.sheenRoughnessTexIdx), uv).a;
    r.params.sheenRoughnessFactor *= mix(shrA, shrB, t);
    r.textures.sheenRoughnessTexIdx = -1;

    // --- Iridescence (r channel) ---
    float irA = 1.0, irB = 1.0;
    if (a.textures.iridescenceTexIdx >= 0) irA = sampleTexture2DLinear(uint(a.textures.iridescenceTexIdx), uv).r;
    if (b.textures.iridescenceTexIdx >= 0) irB = sampleTexture2DLinear(uint(b.textures.iridescenceTexIdx), uv).r;
    r.params.iridescenceFactor *= mix(irA, irB, t);
    r.textures.iridescenceTexIdx = -1;

    // --- Directional / structural maps: threshold pick (A when t<0.5, B otherwise) ---
    r.textures.normalTexIdx                = t < 0.5 ? a.textures.normalTexIdx               : b.textures.normalTexIdx;
    r.textures.occlusionTexIdx             = t < 0.5 ? a.textures.occlusionTexIdx             : b.textures.occlusionTexIdx;
    r.textures.clearcoatNormalTexIdx       = t < 0.5 ? a.textures.clearcoatNormalTexIdx       : b.textures.clearcoatNormalTexIdx;
    r.textures.thicknessTexIdx             = t < 0.5 ? a.textures.thicknessTexIdx             : b.textures.thicknessTexIdx;
    r.textures.anisotropyTexIdx            = t < 0.5 ? a.textures.anisotropyTexIdx            : b.textures.anisotropyTexIdx;
    r.textures.iridescenceThicknessTexIdx  = t < 0.5 ? a.textures.iridescenceThicknessTexIdx  : b.textures.iridescenceThicknessTexIdx;
    r.textures.padding0 = 0;

    return r;
}

// ── Per-light direct pass. Result is multiplied by lightColor externally.
vec3 evaluateUberLightingDirect(Surface s, vec3 L, mat3 baseTBN, PrincipledBSDFData mat, float shadowFactor) {
    BaseLighting direct = evaluateBaseLayerDirect(s, L, shadowFactor);
    vec3 finalColor = direct.diffuse + direct.specular;

    if ((mat.params.featureMask & MAT_FEATURE_SHEEN) != 0u)
        finalColor = applySheenLayer(s, L, finalColor);

    if ((mat.params.featureMask & MAT_FEATURE_CLEARCOAT) != 0u)
        finalColor = applyClearcoatDirect(s, L, baseTBN, finalColor, mat, shadowFactor);

    return finalColor;
}

#endif // MATERIAL_PRINCIPLED_BSDF_GLSL
