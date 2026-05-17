#version 450

layout(location = 0) in vec3 inWorldPos;
layout(location = 1) in vec2 inUV;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec3 inTangent;
layout(location = 4) in vec3 inBitangent;

layout(location = 0) out vec4 outColor;

#include "common/bindless.glsl"
#include "common/environment.glsl"
#include "common/primitive_types.glsl"
#include "common/shadow.glsl"
#include "common/light_types.glsl"

#include "material/diffuse.glsl"
#include "material/diffuse_specular.glsl"
#include "material/principled_bsdf.glsl"
#include "material/mix_material.glsl"

layout(set = 0, binding = 0) uniform Camera {
    mat4 view;
    mat4 proj;
    vec3 position;
} camera;
layout(set = 0, binding = 1) uniform Environment {
    EnvironmentParams params;
} env;
layout(set = 0, binding = 2) uniform SceneParams {
    float exposure;
    float gamma;
    int   shadowsEnabled;
    float shadowBias;
    mat4  shadowViewProj;
} sceneParams;
layout(std430, set = 0, binding = 3) readonly buffer LightsBlock {
    uint  count;
    uint  _pad0; uint _pad1; uint _pad2;
    Light lights[];
} lightsBlock;
layout(set = 0, binding = 4) uniform sampler2DShadow shadowMap;

#include "evaluation.glsl"

layout(std430, set = 2, binding = 0) readonly buffer DiffuseBlock {
    DiffuseData materials[];
} diffData;
layout(std430, set = 2, binding = 1) readonly buffer DiffuseSpecularBlock {
    DiffuseSpecularData materials[];
} diffSpecData;
layout(std430, set = 2, binding = 2) readonly buffer BsdfBlock {
    PrincipledBSDFData materials[];
} bsdfData;
layout(std430, set = 2, binding = 3) readonly buffer MixBlock {
    MixMaterialData materials[];
} mixData;

layout(push_constant) uniform PushConstants {
    mat4 model;
    uint primIndex;
    uint skinOffset;
    uint enableSkinning;
    uint materialType;
    uint materialIndex;
} pcs;

void main() {
    float facing = gl_FrontFacing ? 1.0 : -1.0;
    vec3 N = normalize(inNormal)   * facing;
    vec3 T = length(inTangent)   > 0.1 ? normalize(inTangent)   * facing : vec3(1.0, 0.0, 0.0);
    vec3 B = length(inBitangent) > 0.1 ? normalize(inBitangent) * facing : cross(N, T);
    mat3 baseTBN = mat3(T, B, N);

    vec3 V = normalize(camera.position - inWorldPos);

    if (pcs.materialType == DIFFUSE_MATERIAL) {
        outColor = evaluateDiffuse(diffData.materials[pcs.materialIndex], inUV, baseTBN, inWorldPos);
    } else if (pcs.materialType == DIFFUSE_SPECULAR_MATERIAL) {
        outColor = evaluateDiffuseSpecular(diffSpecData.materials[pcs.materialIndex], inUV, baseTBN, V, inWorldPos);
    } else if (pcs.materialType == PRINCIPLED_MATERIAL) {
        outColor = evaluatePrincipledBSDF(bsdfData.materials[pcs.materialIndex], inWorldPos, inUV, baseTBN, V, env.params, sceneParams.exposure);
        outColor.rgb = pow(outColor.rgb, vec3(1.0 / max(sceneParams.gamma, 0.01)));
    } else if (pcs.materialType == MIX_MATERIAL) {
        MixMaterialData mx = mixData.materials[pcs.materialIndex];
        float f = mx.params.factor;
        if (mx.params.factorTexIdx >= 0)
            f = clamp(sampleTexture2DLinear(uint(mx.params.factorTexIdx), inUV).r, 0.0, 1.0);
        float halfEdge = max(mx.params.edge * 0.5, 1e-5);
        f = clamp(smoothstep(mx.params.threshold - halfEdge, mx.params.threshold + halfEdge, f), 0.0, 1.0);

        // Evaluate both materials in HDR then blend — avoids parameter-space
        // non-linearities (specular humps, featureMask leaking between sides).
        PrincipledBSDFData matA = bsdfData.materials[mx.params.materialIndexA];
        PrincipledBSDFData matB = bsdfData.materials[mx.params.materialIndexB];
        Surface sA = buildSurface(inWorldPos, V, baseTBN, inUV, matA);
        Surface sB = buildSurface(inWorldPos, V, baseTBN, inUV, matB);

        float alpha = mix(sA.albedo.a, sB.albedo.a, f);
        if (mx.params.opacityTexIdx >= 0) {
            float op = sampleTexture2DLinear(uint(mx.params.opacityTexIdx), inUV).r;
            if (op < mx.params.alphaCutoff) discard;
        } else {
            if (alpha < mx.params.alphaCutoff) discard;
        }

        float ambShadow = primaryShadowFactor(inWorldPos);
        vec3 hdrA = evaluateUberLightingIBL(sA, baseTBN, matA, env.params) * mix(0.3, 1.0, ambShadow);
        vec3 hdrB = evaluateUberLightingIBL(sB, baseTBN, matB, env.params) * mix(0.3, 1.0, ambShadow);
        for (uint i = 0; i < lightsBlock.count; i++) {
            vec3 L; vec3 lColor; float shadow;
            computeLightContrib(i, inWorldPos, L, lColor, shadow);
            hdrA += evaluateUberLightingDirect(sA, L, baseTBN, matA, shadow) * lColor;
            hdrB += evaluateUberLightingDirect(sB, L, baseTBN, matB, shadow) * lColor;
        }
        vec3 hdr = tonemapACES(mix(hdrA, hdrB, f) * sceneParams.exposure);
        outColor = vec4(hdr, alpha);
        outColor.rgb = pow(outColor.rgb, vec3(1.0 / max(sceneParams.gamma, 0.01)));
    } else {
        outColor = evaluateFallback(inNormal, inTangent, inUV, inWorldPos);
    }
}
