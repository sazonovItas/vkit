#version 450

#include "common/primitive_types.glsl"
#include "common/primitive_vertex_pulling.glsl"

layout(set = 0, binding = 0) uniform Camera {
    mat4 view;
    mat4 proj;
    vec3 position;
} camera;

layout(std430, set = 3, binding = 0) readonly buffer PrimitiveBlock {
    Primitive primitives[];
} primData;

layout(std430, set = 3, binding = 1) readonly buffer JointBlock {
    mat4 joints[];
} jointData;

layout(push_constant) uniform PushConstants {
    mat4 model;
    uint primIndex;
    uint skinOffset;
    uint enableSkinning;
    uint materialType;
    uint materialIndex;
} pcs;

void main() {
    Primitive prim = primData.primitives[pcs.primIndex];
    vec3 pos    = getPosition(prim, gl_VertexIndex);
    vec3 normal = getNormal(prim, gl_VertexIndex);

    vec4 worldPos;
    vec3 worldNormal;

    if (pcs.enableSkinning == 1 && isAttributeExists(prim.jointIndices[0])) {
        mat4 skinMatrix = mat4(0.0);
        for (uint i = 0; i < 2; ++i) {
            if (isAttributeExists(prim.jointIndices[i]) && isAttributeExists(prim.jointWeights[i])) {
                uvec4 js = getJoints(prim, i, gl_VertexIndex);
                vec4  ws = getWeights(prim, i, gl_VertexIndex);
                skinMatrix +=
                    ws.x * jointData.joints[pcs.skinOffset + js.x] +
                    ws.y * jointData.joints[pcs.skinOffset + js.y] +
                    ws.z * jointData.joints[pcs.skinOffset + js.z] +
                    ws.w * jointData.joints[pcs.skinOffset + js.w];
            }
        }
        worldPos    = skinMatrix * vec4(pos, 1.0);
        worldNormal = normalize(transpose(inverse(mat3(skinMatrix))) * normal);
    } else {
        worldPos    = pcs.model * vec4(pos, 1.0);
        worldNormal = normalize(transpose(inverse(mat3(pcs.model))) * normal);
    }

    // Expand vertex along world normal — back faces of the expanded shell form the outline.
    // Screen-space constant thickness: offset in NDC, scale back to clip-space.
    vec4 clipPos = camera.proj * camera.view * worldPos;
    vec4 clipN   = camera.proj * camera.view * vec4(worldPos.xyz + worldNormal, 1.0);
    vec2 ndcDiff = clipN.xy / clipN.w - clipPos.xy / clipPos.w;
    float ndcLen = length(ndcDiff);
    if (ndcLen > 0.0001)
        clipPos.xy += (ndcDiff / ndcLen) * 0.006 * clipPos.w;

    gl_Position = clipPos;
}
