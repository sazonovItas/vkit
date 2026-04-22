#version 450

layout(location = 0) in vec3 inWorldPos;
layout(location = 1) in vec2 inUV;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec3 inTangent;
layout(location = 4) in vec3 inBitangent;

layout(location = 0) out vec4 outColor;

layout (set = 0, binding = 0) uniform Scene {
} scene;

layout(set = 0, binding = 1) uniform Camera {
    mat4 view;
    mat4 proj;
    vec3 position;
} camera;

layout(set = 1, binding = 0) uniform Material {
    vec4 baseColorFactor;
    vec3 emissiveFactor;
    float metallicFactor;
    float roughnessFactor

    float ior;
    float clearcoatFactor;
    float clearcoatRoughnessFactor;
    float transmissionFactor;

    uint hasBaseColorMap;
    uint hasNormalMap;
    uint hasMetallicRoughnessMap;
    uint hasEmissiveMap;
    uint hasClearcoatMap;
    uint hasClearcoatNormalMap;
} mat;

layout(binding = 1) uniform sampler2D baseColorMap;
layout(binding = 2) uniform sampler2D normalMap;
layout(binding = 3) uniform sampler2D metallicRoughnessMap;
layout(binding = 4) uniform sampler2D emissiveMap;
layout(binding = 5) uniform sampler2D clearcoatMap;
layout(binding = 6) uniform sampler2D clearcoatNormalMap;

const float PI = 3.14159265359;

mat3 calculateTBN(vec3 worldPos, vec2 uv, vec3 normal) {
    vec3 q1 = dFdx(worldPos);
    vec3 q2 = dFdy(worldPos);
    vec2 st1 = dFdx(uv);
    vec2 st2 = dFdy(uv);

    vec3 N = normalize(normal);
    float det = st1.x * st2.y - st2.x * st1.y;
    float invDet = (det != 0.0) ? 1.0 / det : 1.0; 
    
    vec3 T = normalize((q1 * st2.y - q2 * st1.y) * invDet);
    vec3 B = normalize((q2 * st1.x - q1 * st2.x) * invDet);

    T = normalize(T - dot(T, N) * N);
    
    vec3 recomputedB = cross(N, T);
    if (dot(cross(N, T), B) < 0.0) {
        recomputedB *= -1.0;
    }

    return mat3(T, recomputedB, N);
}

float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float denom = (NdotH * NdotH * (a2 - 1.0) + 1.0);
    return a2 / (PI * denom * denom);
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx1 = NdotV / (NdotV * (1.0 - k) + k);
    float ggx2 = NdotL / (NdotL * (1.0 - k) + k);
    return ggx1 * ggx2;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

float fresnelSchlickClearcoat(float cosTheta) {
    float F0 = 0.04; 
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

void main() {
    vec4 albedo = mat.baseColorFactor;
    if (mat.hasBaseColorMap == 1) albedo *= texture(baseColorMap, inUV);

    float metallic = mat.metallicFactor;
    float roughness = mat.roughnessFactor;
    if (mat.hasMetallicRoughnessMap == 1) {
        vec4 mr = texture(metallicRoughnessMap, inUV);
        metallic *= mr.b;
        roughness *= mr.g;
    }

    vec3 emissive = mat.emissiveFactor;
    if (mat.hasEmissiveMap == 1) {
        emissive *= texture(emissiveMap, inUV).rgb;
    }

    vec3 N = normalize(inNormal);
    vec3 coatN = N; 
    
    mat3 TBN;
    if (length(inTangent) > 0.1 && length(inBitangent) > 0.1) {
        TBN = mat3(normalize(inTangent), normalize(inBitangent), N);
    } else {
        TBN = calculateTBN(inWorldPos, inUV, N);
    }

    if (mat.hasNormalMap == 1) {
        N = normalize(TBN * (texture(normalMap, inUV).xyz * 2.0 - 1.0));
    }
    if (mat.hasClearcoatNormalMap == 1) {
        coatN = normalize(TBN * (texture(clearcoatNormalMap, inUV).xyz * 2.0 - 1.0));
    }

    float iorRatio = (mat.ior - 1.0) / (mat.ior + 1.0);
    vec3 F0 = vec3(iorRatio * iorRatio); 
    F0 = mix(F0, albedo.rgb, metallic);

    vec3 V = normalize(camera.position - inWorldPos);
    vec3 L = normalize(vec3(1.0, 1.0, 1.0));
    vec3 H = normalize(V + L);
    float NdotL = max(dot(N, L), 0.0);
    float NdotV = max(dot(N, V), 0.0);
    float VdotH = max(dot(V, H), 0.0);

    float NDF = DistributionGGX(N, H, roughness);   
    float G   = GeometrySmith(N, V, L, roughness);      
    vec3 F    = fresnelSchlick(max(dot(H, V), 0.0), F0);       

    vec3 baseSpecular = (NDF * G * F) / (4.0 * NdotV * NdotL + 0.0001);
    
    vec3 kD = (vec3(1.0) - F) * (1.0 - metallic);
    vec3 baseDiffuse = (kD * albedo.rgb / PI);
    
    vec3 baseLayerColor = (baseDiffuse + baseSpecular) * vec3(1.0) * NdotL;

    float coatRoughness = mat.clearcoatRoughnessFactor;
    float coatFactor = mat.clearcoatFactor;
    if (mat.hasClearcoatMap == 1) coatFactor *= texture(clearcoatMap, inUV).r;

    if (coatFactor > 0.0) {
        float coatNDF = DistributionGGX(coatN, H, coatRoughness);
        float coatG = GeometrySmith(coatN, V, L, coatRoughness);
        float coatF = fresnelSchlickClearcoat(VdotH);

        float coatSpec = (coatNDF * coatG * coatF) / (4.0 * max(dot(coatN, V), 0.0) * max(dot(coatN, L), 0.0) + 0.0001);
        
        vec3 coatBlendWeight = vec3(coatFactor * coatF);
        baseLayerColor = baseLayerColor * (vec3(1.0) - coatBlendWeight) + (coatSpec * coatFactor * vec3(1.0) * max(dot(coatN, L), 0.0));
    }

    vec3 ambient = vec3(0.03) * albedo.rgb * (1.0 - metallic); 
    vec3 finalColor = ambient + baseLayerColor + emissive;
    
    finalColor = finalColor / (finalColor + vec3(1.0));
    finalColor = pow(finalColor, vec3(1.0/2.2));

    outColor = vec4(finalColor, albedo.a);
}
