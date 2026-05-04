#ifndef EVALUATION_GLSL
#define EVALUATION_GLSL

#include "common/environment.glsl"

#define DIFFUSE_MATERIAL          1
#define DIFFUSE_SPECULAR_MATERIAL 2
#define PRINCIPLED_MATERIAL       3

const vec3 LIGHT_COLOR = vec3(1.0, 1.0, 1.0);
const vec3 AMBIENT_COLOR = vec3(0.05, 0.05, 0.05);
const vec3 LIGHT_DIR = normalize(vec3(-1.0, -1.0, -1.0));

vec4 evaluateFallback(vec3 vertNormal, vec3 vertTangent, vec2 uv) {
    vec3 normal = normalize(vertNormal);
    
    if (length(normal) < 0.1) {
        normal = vec3(0.0, 1.0, 0.0);
    }

    float diff = max(dot(normal, -LIGHT_DIR), 0.0);
    
    vec3 albedo = vec3(0.2, 0.5, 0.8);
    vec3 ambient = albedo * 0.1;
    vec3 finalColor = ambient + (albedo * diff);

    // finalColor = vec3(uv, 0.0); 
    // finalColor = normal * 0.5 + 0.5; 
    // finalColor = normalize(vertTangent) * 0.5 + 0.5;

    return vec4(finalColor, 1.0);
}

vec4 evaluateDiffuse(DiffuseData mat, vec2 uv, mat3 baseTBN, vec3 L) {
    vec4 albedo = mat.params.diffuseFactor;
    if (mat.textures.diffuseTexIdx >= 0) {
        albedo *= sampleTexture2DLinear(uint(mat.textures.diffuseTexIdx), uv);
    }

    if (albedo.a < 0.01) discard;

    vec3 N = baseTBN[2];
    if (mat.textures.normalTexIdx >= 0) {
        vec3 normalSample = sampleTexture2DLinear(uint(mat.textures.normalTexIdx), uv).xyz;
        N = normalize(baseTBN * (normalSample * 2.0 - 1.0));
    }

    float NdotL = max(dot(N, L), 0.0);
    vec3 finalColor = (AMBIENT_COLOR + (LIGHT_COLOR * NdotL)) * albedo.rgb;

    return vec4(finalColor, albedo.a);
}

vec4 evaluateDiffuseSpecular(DiffuseSpecularData mat, vec2 uv, mat3 baseTBN, vec3 V, vec3 L) {
    vec4 albedo = mat.params.diffuseFactor;
    if (mat.textures.diffuseTexIdx >= 0) {
        albedo *= sampleTexture2DLinear(uint(mat.textures.diffuseTexIdx), uv);
    }

    if (albedo.a < 0.01) discard;

    vec3 N = baseTBN[2];
    if (mat.textures.normalTexIdx >= 0) {
        vec3 normalSample = sampleTexture2DLinear(uint(mat.textures.normalTexIdx), uv).xyz;
        N = normalize(baseTBN * (normalSample * 2.0 - 1.0));
    }

    vec3 specularColor = mat.params.specularFactor;
    float glossiness = mat.params.glossinessFactor;
    if (mat.textures.specularGlossinessTexIdx >= 0) {
        vec4 specGlossMap = sampleTexture2DLinear(uint(mat.textures.specularGlossinessTexIdx), uv);
        specularColor *= specGlossMap.rgb;
        glossiness *= specGlossMap.a;
    }

    float shininess = exp2(glossiness * 10.0 + 1.0);
    vec3 H = normalize(L + V);
    float NdotL = max(dot(N, L), 0.0);
    float NdotH = max(dot(N, H), 0.0);

    vec3 diffuseLighting = albedo.rgb * NdotL;
    vec3 specularLighting = NdotL > 0.0 ? specularColor * pow(NdotH, shininess) : vec3(0.0);

    vec3 finalColor = (AMBIENT_COLOR * albedo.rgb) + (LIGHT_COLOR * (diffuseLighting + specularLighting));
    return vec4(finalColor, albedo.a);
}

vec4 evaluatePrincipledBSDF(PrincipledBSDFData mat, vec3 worldPos, vec2 uv, mat3 baseTBN, vec3 V, vec3 L, EnvironmentParams envParams) {
    Surface s = buildSurface(worldPos, V, baseTBN, uv, mat);

    if (s.albedo.a < 0.01) discard;

    vec3 finalColor = evaluateUberLighting(s, L, baseTBN, mat, envParams);
    return vec4(finalColor, s.albedo.a);
}

#endif // EVALUATION_GLSL
