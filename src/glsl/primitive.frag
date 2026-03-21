#version 450 core

#include "pcs.glsl"
#include "material.glsl"
#include "bindless.glsl"
#include "light.glsl" 

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
    float exposure;
    float gamma;
    float iblIntensity;

    int diffuseEnvMapIdx;
    int specularEnvMapIdx;
    float maxSpecularLod;

    int lightCount;
} uboParams;

layout (set = 0, binding = 2, std430) readonly buffer SSBOLights {
    Light lights[];
};

layout (set = 1, binding = 0, std430) readonly buffer SSBOMaterials {
    Material materials[];
};

layout (location = 0) out vec4 outColor;

const float PI = 3.14159265358979323846;
const vec2 INV_ATAN = vec2(0.15915494309, 0.31830988618);

vec3 sRGBToLinear(vec3 srgb) { return pow(srgb, vec3(2.2)); }

vec2 directionToSphericalEnvmap(vec3 dir) {
    vec2 uv = vec2(atan(dir.z, dir.x), asin(dir.y));
    uv *= INV_ATAN;
    uv += 0.5;
    uv.y = 1.0 - uv.y;
    return uv;
}

vec3 ACESFilm(vec3 x) {
    float a = 2.51;
    float b = 0.03;
    float c = 2.43;
    float d = 0.59;
    float e = 0.14;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

vec3 getNormalFromMap(int texIdx, vec2 uv, vec3 worldPos, vec3 vertexNormal) {
    vec3 tangentNormal = sampleTexture2DLinear(texIdx, uv).xyz * 2.0 - 1.0;

    vec3 q1 = dFdx(worldPos);
    vec3 q2 = dFdy(worldPos);
    vec2 st1 = dFdx(uv);
    vec2 st2 = dFdy(uv);

    vec3 N = normalize(vertexNormal);
    vec3 T = normalize(q1 * st2.t - q2 * st1.t);
    vec3 B = -normalize(cross(N, T));
    mat3 TBN = mat3(T, B, N);

    return normalize(TBN * tangentNormal);
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
    roughness = clamp(roughness, 0.0, 1.0);
    metallic = clamp(metallic, 0.0, 1.0);

    vec3 N = normalize(inNormal);
    if (mat.normalTextureIdx != -1) {
        N = getNormalFromMap(mat.normalTextureIdx, inUV0, inWorldPos, N);
    }

    float ao = 1.0;
    if (mat.occlusionTextureIdx != -1) {
        ao = sampleTexture2DLinear(mat.occlusionTextureIdx, inUV0).r;
    }

    vec3 emissive = mat.emissiveFactor.rgb;
    if (mat.emissiveTextureIdx != -1) {
        emissive *= sRGBToLinear(sampleTexture2DLinear(mat.emissiveTextureIdx, inUV0).rgb);
    }
    emissive *= mat.emissiveStrength;

    vec3 V = normalize(ubo.cameraPosition - inWorldPos);
    vec3 R = reflect(-V, N);
    vec3 F0 = mix(vec3(0.04), albedo.rgb, metallic);
    float NdotV = max(dot(N, V), 0.0);

    vec3 Lo = vec3(0.0);
    
    for(int i = 0; i < uboParams.lightCount; ++i) {
        Light light = lights[i];
        
        vec3 L;
        float attenuation = 1.0;
        
        if (light.type == TYPE_DIRECTIONAL_LIGHT) {
            L = normalize(-light.direction);
        } else {
            vec3 lightVec = light.position - inWorldPos;
            float dist = length(lightVec);
            L = normalize(lightVec);
            
            float attenuationFactor = 1.0;
            if (light.range > 0.0) {
                float distOverRange = dist / light.range;
                float distOverRange4 = distOverRange * distOverRange * distOverRange * distOverRange;
                attenuationFactor = clamp(1.0 - distOverRange4, 0.0, 1.0);
            }
            attenuation = attenuationFactor / (dist * dist + 0.0001);
            
            if (light.type == TYPE_SPOT_LIGHT) {
                float currentCos = dot(L, normalize(-light.direction));
                float spotAttenuation = clamp(currentCos * light.scaleOffset.x + light.scaleOffset.y, 0.0, 1.0);
                attenuation *= spotAttenuation;
            }
        }
        
        vec3 radiance = light.color * light.intensity * attenuation;
        vec3 H = normalize(V + L);
        float NdotL = max(dot(N, L), 0.0);
        
        float NDF = DistributionGGX(N, H, roughness);
        float G   = GeometrySmith(N, V, L, roughness);
        vec3 F    = fresnelSchlick(max(dot(H, V), 0.0), F0);
        
        vec3 numerator = NDF * G * F;
        float denominator = 4.0 * NdotV * NdotL + 0.0001;
        vec3 specularDirect = numerator / denominator;
        
        vec3 kS = F;
        vec3 kD = vec3(1.0) - kS;
        kD *= 1.0 - metallic;
        
        Lo += (kD * albedo.rgb / PI + specularDirect) * radiance * NdotL;
    }

    vec3 ambient = vec3(0.0);
    vec3 diffuseIBL = vec3(0.03);
    vec3 specularIBL = vec3(0.0);
    
    vec2 envBRDF = vec2(1.0);
    vec4 c0 = vec4(-1.0, -0.0275, -0.572, 0.022);
    vec4 c1 = vec4(1.0, 0.0425, 1.04, -0.04);
    vec4 r = roughness * c0 + c1;
    float a004 = min(r.x * r.x, exp2(-9.28 * NdotV)) * r.x + r.y;
    envBRDF = vec2(-1.04, 1.04) * a004 + r.zw;
    
    vec3 F = fresnelSchlick(max(dot(N, V), 0.0), F0);
    vec3 kD_IBL = 1.0 - F;
    kD_IBL *= 1.0 - metallic;

    if (uboParams.diffuseEnvMapIdx != -1) {
        vec2 diffuseUV = directionToSphericalEnvmap(N);
        diffuseIBL = sampleTexture2DLinear(uboParams.diffuseEnvMapIdx, diffuseUV).rgb;
        
        diffuseIBL = diffuseIBL * albedo.rgb;
    }

    if (uboParams.specularEnvMapIdx != -1) {
        vec2 specularUV = directionToSphericalEnvmap(R);
        float lod = roughness * uboParams.maxSpecularLod;
        vec3 prefilteredColor = sampleTexture2DLod(uboParams.specularEnvMapIdx, specularUV, lod).rgb;
        
        vec3 F_Spec = F0 * envBRDF.x + envBRDF.y;
        specularIBL = prefilteredColor * F_Spec;
    }
    
    ambient = (kD_IBL * diffuseIBL) + specularIBL;
    ambient *= ao * uboParams.iblIntensity;

    vec3 sceneColor = ambient + Lo + emissive;
    
    sceneColor *= uboParams.exposure;
    sceneColor = ACESFilm(sceneColor);
    sceneColor = pow(sceneColor, vec3(1.0 / uboParams.gamma));

    float alpha = clamp(albedo.a + mat.dissolveStrength, 0.0, 1.0);
    outColor = vec4(sceneColor, alpha);
}
