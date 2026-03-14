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

    int diffuseEnvMapIdx;
    int specularEnvMapIdx;
    int brdfLUTIdx;
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

vec3 sRGBToLinear(vec3 srgb) { return pow(srgb, vec3(2.2)); }

vec3 linearToSRGB(vec3 linear) { return pow(linear, vec3(1.0 / uboParams.gamma)); }

vec2 directionToSphericalEnvmap(vec3 dir) {
    float s = 1.0 - mod(1.0 / (2.0 * PI) * atan(dir.y, dir.x), 1.0);
    float t = 1.0 / PI * acos(-dir.z);
    return vec2(s, t);
}

vec3 getNormalFromMap(int texIdx, vec2 uv, vec3 worldPos, vec3 vertexNormal) {
    vec3 tangentNormal = sampleTexture2DLinear(texIdx, uv).xyz * 2.0 - 1.0;
    vec3 dp1 = dFdx(worldPos); vec3 dp2 = dFdy(worldPos);
    vec2 duv1 = dFdx(uv); vec2 duv2 = dFdy(uv);
    vec3 dp2perp = cross(dp2, vertexNormal); vec3 dp1perp = cross(vertexNormal, dp1);
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
    if (uboParams.brdfLUTIdx != -1) {
        envBRDF = sampleTexture2DLinear(uboParams.brdfLUTIdx, vec2(NdotV, roughness)).rg;
    } else {
        vec4 c0 = vec4(-1.0, -0.0275, -0.572, 0.022);
        vec4 c1 = vec4(1.0, 0.0425, 1.04, -0.04);
        vec4 r = roughness * c0 + c1;
        float a004 = min(r.x * r.x, exp2(-9.28 * NdotV)) * r.x + r.y;
        envBRDF = vec2(-1.04, 1.04) * a004 + r.zw;
    }
    
    vec3 F_IBL = F0 * envBRDF.x + envBRDF.y;

    if (uboParams.diffuseEnvMapIdx != -1) {
        vec2 diffuseUV = directionToSphericalEnvmap(N);
        diffuseIBL = sampleTexture2DLinear(uboParams.diffuseEnvMapIdx, diffuseUV).rgb;
    }

    if (uboParams.specularEnvMapIdx != -1) {
        vec2 specularUV = directionToSphericalEnvmap(R);
        float lod = roughness * uboParams.maxSpecularLod;
        vec3 prefilteredColor = sampleTexture2DLod(uboParams.specularEnvMapIdx, specularUV, lod).rgb;
        specularIBL = prefilteredColor * F_IBL;
    }
    
    vec3 kD_IBL = 1.0 - F_IBL;
    kD_IBL *= 1.0 - metallic;
    
    ambient = (kD_IBL * diffuseIBL * albedo.rgb) + specularIBL;
    ambient *= ao;

    vec3 sceneColor = ambient + Lo + emissive;
    sceneColor *= uboParams.exposure;

    outColor = vec4(linearToSRGB(sceneColor), albedo.a + 0.3);
}
