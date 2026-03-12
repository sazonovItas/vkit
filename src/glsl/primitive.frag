#version 450 core

#include "pcs.glsl"
#include "material.glsl"
#include "bindless.glsl"

layout (location = 0) in vec3 inWorldPos;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inUV0;

layout (set = 0, binding = 0) uniform UBO {
    mat4 model;
    mat4 view;
    mat4 projection;
    vec3 cameraPosition;
} ubo;

layout (set = 0, binding = 1) uniform UBOParams {
    vec3 lightDir;
    float exposure;
    float gamma;
} uboParams;

layout (set = 1, binding = 0, std430) readonly buffer SSBO {
    Material materials[];
};

layout (location = 0) out vec4 outColor;

const float PI = 3.14159265359;

vec3 ACESFilm(vec3 x) {
    float a = 2.51;
    float b = 0.03;
    float c = 2.43;
    float d = 0.59;
    float e = 0.14;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

vec3 sRGBToLinear(vec3 srgb) {
    return pow(srgb, vec3(uboParams.gamma));
}

vec3 linearToSRGB(vec3 linear) {
    return pow(linear, vec3(1.0 / uboParams.gamma));
}

vec3 getNormalFromMap(int texIdx, vec2 uv, vec3 worldPos, vec3 vertexNormal) {
    vec3 tangentNormal = sampleTexture2DLinear(texIdx, uv).xyz * 2.0 - 1.0;
    vec3 dp1 = dFdx(worldPos);
    vec3 dp2 = dFdy(worldPos);
    vec2 duv1 = dFdx(uv);
    vec2 duv2 = dFdy(uv);
    vec3 dp2perp = cross(dp2, vertexNormal);
    vec3 dp1perp = cross(vertexNormal, dp1);
    vec3 T = dp2perp * duv1.x + dp1perp * duv2.x;
    vec3 B = dp2perp * duv1.y + dp1perp * duv2.y;
    float maxVal = max(dot(T, T), dot(B, B));
    if (maxVal <= 0.00001) return normalize(vertexNormal);
    return normalize(mat3(T * inversesqrt(maxVal), B * inversesqrt(maxVal), vertexNormal) * tangentNormal);
}

float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    return a2 / (PI * pow(NdotH * NdotH * (a2 - 1.0) + 1.0, 2.0));
}

float GeometrySchlickGGX(float NdotV, float roughness) {
    float k = pow(roughness + 1.0, 2.0) / 8.0;
    return NdotV / (NdotV * (1.0 - k) + k);
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    return GeometrySchlickGGX(max(dot(N, V), 0.0), roughness) * GeometrySchlickGGX(max(dot(N, L), 0.0), roughness);
}

vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

void main() {
    Material mat = materials[pcs.materialIdx];
    

    vec4 albedoSample = vec4(1.0);
    if (mat.baseColorTextureIdx != -1) {
        albedoSample = sampleTexture2DLinear(mat.baseColorTextureIdx, inUV0);
        albedoSample.rgb = sRGBToLinear(albedoSample.rgb);
    }
    vec4 albedo = albedoSample * mat.baseColorFactor;
    
    if (albedo.a < mat.alphaMaskCutoff) discard;

    float metallic = mat.metallicFactor;
    float roughness = mat.roughnessFactor;
    if (mat.metallicRoughnessTextureIdx != -1) {
        vec4 mr = sampleTexture2DLinear(mat.metallicRoughnessTextureIdx, inUV0);
        roughness *= mr.g; 
        metallic *= mr.b;
    }
    roughness = clamp(roughness, 0.05, 1.0);
    metallic = clamp(metallic, 0.0, 1.0);

    vec3 N = normalize(inNormal);
    if (mat.normalTextureIdx != -1) N = getNormalFromMap(mat.normalTextureIdx, inUV0, inWorldPos, N);

    vec3 emissive = mat.emissiveFactor.rgb;
    if (mat.emissiveTextureIdx != -1) {
        vec3 eSample = sampleTexture2DLinear(mat.emissiveTextureIdx, inUV0).rgb;
        emissive *= sRGBToLinear(eSample);
    }
    emissive *= mat.emissiveStrength;

    vec3 V = normalize(ubo.cameraPosition - inWorldPos);
    vec3 L = normalize(-uboParams.lightDir);
    vec3 H = normalize(V + L);
    
    vec3 radiance = vec3(5.0); 
    vec3 F0 = mix(vec3(0.04), albedo.rgb, metallic);

    float NDF = DistributionGGX(N, H, roughness);
    float G   = GeometrySmith(N, V, L, roughness);
    vec3 F    = fresnelSchlick(max(dot(H, V), 0.0), F0);

    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
    vec3 specular = numerator / denominator;

    vec3 kS = F;
    vec3 kD = (vec3(1.0) - kS) * (1.0 - metallic);
    
    vec3 Lo = (kD * albedo.rgb / PI + specular) * radiance * max(dot(N, L), 0.0);

    vec3 ambient = vec3(0.03) * albedo.rgb; 
    vec3 sceneColor = ambient + Lo + emissive;

    sceneColor *= uboParams.exposure;
    sceneColor = ACESFilm(sceneColor);

    outColor = vec4(linearToSRGB(sceneColor), albedo.a);
}
