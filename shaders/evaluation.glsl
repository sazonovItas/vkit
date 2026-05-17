#ifndef EVALUATION_GLSL
#define EVALUATION_GLSL

#include "common/environment.glsl"
#include "common/lights.glsl"

vec3 tonemapACES(vec3 x) {
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

#define DIFFUSE_MATERIAL          1
#define DIFFUSE_SPECULAR_MATERIAL 2
#define PRINCIPLED_MATERIAL       3
#define MIX_MATERIAL              4

const vec3 AMBIENT_COLOR = vec3(0.05, 0.05, 0.05);

// Returns the shadow factor from the first shadow-casting light, or 1.0 if none.
float primaryShadowFactor(vec3 worldPos) {
    if (sceneParams.shadowsEnabled == 0) return 1.0;
    for (uint i = 0; i < lightsBlock.count; i++) {
        if (lightsBlock.lights[i].castsShadows == 0) continue;
        vec3 L; vec3 lColor; float shadow;
        computeLightContrib(i, worldPos, L, lColor, shadow);
        return shadow;
    }
    return 1.0;
}

vec4 evaluateFallback(vec3 vertNormal, vec3 vertTangent, vec2 uv, vec3 worldPos) {
    vec3 normal = normalize(length(vertNormal) > 0.1 ? vertNormal : vec3(0.0, 1.0, 0.0));

    float ambShadow = primaryShadowFactor(worldPos);
    vec3 totalLight = vec3(0.0);
    for (uint i = 0; i < lightsBlock.count; i++) {
        vec3 L; vec3 lColor; float shadow;
        computeLightContrib(i, worldPos, L, lColor, shadow);
        totalLight += lColor * max(dot(normal, L), 0.0) * shadow;
    }
    if (lightsBlock.count == 0u)
        totalLight = vec3(max(dot(normal, normalize(vec3(-1.0, -1.0, -1.0))), 0.0));

    vec3 albedo = vec3(0.2, 0.5, 0.8);
    return vec4(albedo * AMBIENT_COLOR * mix(0.3, 1.0, ambShadow) + albedo * totalLight, 1.0);
}

vec4 evaluateDiffuse(DiffuseData mat, vec2 uv, mat3 baseTBN, vec3 worldPos) {
    vec4 albedo = mat.params.diffuseFactor;
    if (mat.textures.diffuseTexIdx >= 0)
        albedo *= sampleTexture2DLinear(uint(mat.textures.diffuseTexIdx), uv);
    if (albedo.a < 0.01) discard;

    vec3 N = baseTBN[2];
    if (mat.textures.normalTexIdx >= 0) {
        vec3 ns = sampleTexture2DLinear(uint(mat.textures.normalTexIdx), uv).xyz;
        N = normalize(baseTBN * (ns * 2.0 - 1.0));
    }

    float ambShadow = primaryShadowFactor(worldPos);
    vec3 directLight = vec3(0.0);
    for (uint i = 0; i < lightsBlock.count; i++) {
        vec3 L; vec3 lColor; float shadow;
        computeLightContrib(i, worldPos, L, lColor, shadow);
        directLight += lColor * max(dot(N, L), 0.0) * shadow;
    }

    return vec4((AMBIENT_COLOR * mix(0.3, 1.0, ambShadow) + directLight) * albedo.rgb, albedo.a);
}

vec4 evaluateDiffuseSpecular(DiffuseSpecularData mat, vec2 uv, mat3 baseTBN, vec3 V, vec3 worldPos) {
    vec4 albedo = mat.params.diffuseFactor;
    if (mat.textures.diffuseTexIdx >= 0)
        albedo *= sampleTexture2DLinear(uint(mat.textures.diffuseTexIdx), uv);
    if (albedo.a < 0.01) discard;

    vec3 N = baseTBN[2];
    if (mat.textures.normalTexIdx >= 0) {
        vec3 ns = sampleTexture2DLinear(uint(mat.textures.normalTexIdx), uv).xyz;
        N = normalize(baseTBN * (ns * 2.0 - 1.0));
    }

    vec3 specularColor = mat.params.specularFactor;
    float glossiness   = mat.params.glossinessFactor;
    if (mat.textures.specularGlossinessTexIdx >= 0) {
        vec4 sg = sampleTexture2DLinear(uint(mat.textures.specularGlossinessTexIdx), uv);
        specularColor *= sg.rgb;
        glossiness    *= sg.a;
    }
    float shininess = exp2(glossiness * 10.0 + 1.0);

    float ambShadow = primaryShadowFactor(worldPos);
    vec3 finalColor = AMBIENT_COLOR * mix(0.3, 1.0, ambShadow) * albedo.rgb;
    for (uint i = 0; i < lightsBlock.count; i++) {
        vec3 L; vec3 lColor; float shadow;
        computeLightContrib(i, worldPos, L, lColor, shadow);

        vec3 H = normalize(L + V);
        float NdotL = max(dot(N, L), 0.0);
        float NdotH = max(dot(N, H), 0.0);

        vec3 diffuse  = albedo.rgb * NdotL;
        vec3 specular = NdotL > 0.0 ? specularColor * pow(NdotH, shininess) : vec3(0.0);

        finalColor += lColor * (diffuse + specular) * shadow;
    }

    return vec4(finalColor, albedo.a);
}

vec4 evaluatePrincipledBSDF(PrincipledBSDFData mat, vec3 worldPos, vec2 uv,
                             mat3 baseTBN, vec3 V,
                             EnvironmentParams envParams, float exposure) {
    Surface s = buildSurface(worldPos, V, baseTBN, uv, mat);
    if (s.albedo.a < 0.01) discard;

    // IBL ambient — attenuated by primary shadow so shadowed areas lose ambient too.
    float ambShadow = primaryShadowFactor(worldPos);
    vec3 finalColor = evaluateUberLightingIBL(s, baseTBN, mat, envParams) * mix(0.3, 1.0, ambShadow);

    // Direct contribution — accumulated over all lights.
    for (uint i = 0; i < lightsBlock.count; i++) {
        vec3 L; vec3 lColor; float shadow;
        computeLightContrib(i, worldPos, L, lColor, shadow);
        finalColor += evaluateUberLightingDirect(s, L, baseTBN, mat, shadow) * lColor;
    }

    finalColor = tonemapACES(finalColor * exposure);
    return vec4(finalColor, s.albedo.a);
}

#endif // EVALUATION_GLSL
