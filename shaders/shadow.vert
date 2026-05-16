#version 450

#include "common/primitive_types.glsl"
#include "common/primitive_vertex_pulling.glsl"

layout(set = 0, binding = 0) uniform ShadowLight {
    mat4 viewProj;
} light;

layout(std430, set = 1, binding = 0) readonly buffer PrimitiveBlock {
    Primitive primitives[];
} primData;

layout(std430, set = 1, binding = 1) readonly buffer JointBlock {
    mat4 joints[];
} jointData;

layout(push_constant) uniform PushConstants {
    mat4 model;
    uint primIndex;
    uint skinOffset;
    uint enableSkinning;
    uint _pad;
} pcs;

void main() {
    Primitive prim = primData.primitives[pcs.primIndex];
    vec3 pos = getPosition(prim, gl_VertexIndex);

    vec4 worldPos;
    if (pcs.enableSkinning == 1 && isAttributeExists(prim.jointIndices[0])) {
        mat4 skinMatrix = mat4(0.0);
        for (uint i = 0; i < 2; ++i) {
            if (isAttributeExists(prim.jointIndices[i]) && isAttributeExists(prim.jointWeights[i])) {
                uvec4 j = getJoints(prim, i, gl_VertexIndex);
                vec4 w  = getWeights(prim, i, gl_VertexIndex);
                skinMatrix +=
                    w.x * jointData.joints[pcs.skinOffset + j.x] +
                    w.y * jointData.joints[pcs.skinOffset + j.y] +
                    w.z * jointData.joints[pcs.skinOffset + j.z] +
                    w.w * jointData.joints[pcs.skinOffset + j.w];
            }
        }
        worldPos = skinMatrix * vec4(pos, 1.0);
    } else {
        worldPos = pcs.model * vec4(pos, 1.0);
    }

    gl_Position = light.viewProj * worldPos;
}
