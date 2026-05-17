#version 450

layout(location = 0) in vec3 inLocalPos;
layout(location = 1) in vec3 inWorldPos;

layout(location = 0) out vec4 outColor;

#include "ray_sphere.glsl"

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
    uint materialType;
    uint materialIndex;
    uint enableDepthWrite;
} pcs;

void main() {
    SphereHit hit = calculateSphereHit(inLocalPos, camera.position, pcs.model, camera.view, camera.proj, pcs.enableDepthWrite);

    vec3 V       = normalize(camera.position - hit.worldPos);
    mat3 baseTBN = mat3(hit.tangent, hit.bitangent, hit.normal);

    if (pcs.materialType == DIFFUSE_MATERIAL) {
        outColor = evaluateDiffuse(diffData.materials[pcs.materialIndex], hit.uv, baseTBN, hit.worldPos);
    } else if (pcs.materialType == DIFFUSE_SPECULAR_MATERIAL) {
        outColor = evaluateDiffuseSpecular(diffSpecData.materials[pcs.materialIndex], hit.uv, baseTBN, V, hit.worldPos);
    } else if (pcs.materialType == PRINCIPLED_MATERIAL) {
        outColor = evaluatePrincipledBSDF(bsdfData.materials[pcs.materialIndex], hit.worldPos, hit.uv, baseTBN, V, env.params, sceneParams.exposure);
        outColor.rgb = pow(outColor.rgb, vec3(1.0 / max(sceneParams.gamma, 0.01)));
    } else if (pcs.materialType == MIX_MATERIAL) {
        MixMaterialData mx = mixData.materials[pcs.materialIndex];
        float f = mx.params.factor;
        if (mx.params.factorTexIdx >= 0)
            f = clamp(sampleTexture2DLinear(uint(mx.params.factorTexIdx), hit.uv).r, 0.0, 1.0);
        float halfEdge = max(mx.params.edge * 0.5, 1e-5);
        f = clamp(smoothstep(mx.params.threshold - halfEdge, mx.params.threshold + halfEdge, f), 0.0, 1.0);

        PrincipledBSDFData matA = bsdfData.materials[mx.params.materialIndexA];
        PrincipledBSDFData matB = bsdfData.materials[mx.params.materialIndexB];
        Surface sA = buildSurface(hit.worldPos, V, baseTBN, hit.uv, matA);
        Surface sB = buildSurface(hit.worldPos, V, baseTBN, hit.uv, matB);

        float alpha = mix(sA.albedo.a, sB.albedo.a, f);
        if (mx.params.opacityTexIdx >= 0) {
            float op = sampleTexture2DLinear(uint(mx.params.opacityTexIdx), hit.uv).a;
            if (op < mx.params.alphaCutoff) discard;
        } else {
            if (alpha < mx.params.alphaCutoff) discard;
        }

        float ambShadow = primaryShadowFactor(hit.worldPos);
        vec3 hdrA = evaluateUberLightingIBL(sA, baseTBN, matA, env.params) * mix(0.3, 1.0, ambShadow);
        vec3 hdrB = evaluateUberLightingIBL(sB, baseTBN, matB, env.params) * mix(0.3, 1.0, ambShadow);
        for (uint i = 0; i < lightsBlock.count; i++) {
            vec3 L; vec3 lColor; float shadow;
            computeLightContrib(i, hit.worldPos, L, lColor, shadow);
            hdrA += evaluateUberLightingDirect(sA, L, baseTBN, matA, shadow) * lColor;
            hdrB += evaluateUberLightingDirect(sB, L, baseTBN, matB, shadow) * lColor;
        }
        vec3 hdr = tonemapACES(mix(hdrA, hdrB, f) * sceneParams.exposure);
        outColor = vec4(hdr, alpha);
        outColor.rgb = pow(outColor.rgb, vec3(1.0 / max(sceneParams.gamma, 0.01)));
    } else {
        outColor = evaluateFallback(hit.normal, hit.tangent, hit.uv, hit.worldPos);
    }
}
