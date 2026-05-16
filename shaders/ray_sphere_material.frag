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
    } else {
        outColor = evaluateFallback(hit.normal, hit.tangent, hit.uv, hit.worldPos);
    }
}
