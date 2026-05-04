#version 450

layout(location = 0) out vec3 outWorldPos;
layout(location = 1) out vec2 outUV;
layout(location = 2) out vec3 outNormal;
layout(location = 3) out vec3 outTangent;
layout(location = 4) out vec3 outBitangent;

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
} pcs;

void main() {
    Primitive prim = primData.primitives[pcs.primIndex];

    vec3 pos = getPosition(prim, gl_VertexIndex);
    vec3 normal = getNormal(prim, gl_VertexIndex);

    vec2 uv = vec2(0.0);
    if (isAttributeExists(prim.texcoords[0])) {
        uv = getTexcoord(prim, 0, gl_VertexIndex);
    }

    vec4 worldPos;
    mat3 normalMatrix;

    if (pcs.enableSkinning == 1 && isAttributeExists(prim.jointIndices[0])) {
        mat4 skinMat = mat4(0.0);

        for (uint i = 0; i < 2; ++i) {
            if (isAttributeExists(prim.jointIndices[i]) && isAttributeExists(prim.jointWeights[i])) {
                uvec4 joints = getJoints(prim, i, gl_VertexIndex);
                vec4 weights = getWeights(prim, i, gl_VertexIndex);

                skinMat +=
                    weights.x * jointData.joints[pcs.skinOffset + joints.x] +
                        weights.y * jointData.joints[pcs.skinOffset + joints.y] +
                        weights.z * jointData.joints[pcs.skinOffset + joints.z] +
                        weights.w * jointData.joints[pcs.skinOffset + joints.w];
            }
        }

        worldPos = skinMat * vec4(pos, 1.0);
        normalMatrix = transpose(inverse(mat3(skinMat)));
    } else {
        worldPos = pcs.model * vec4(pos, 1.0);
        normalMatrix = transpose(inverse(mat3(pcs.model)));
    }

    outWorldPos = worldPos.xyz;
    outUV = uv;
    outNormal = normalize(normalMatrix * normal);

    outTangent = vec3(0.0);
    outBitangent = vec3(0.0);
    if (isAttributeExists(prim.tangent)) {
        vec4 tangent = getTangent(prim, gl_VertexIndex);
        outTangent = normalize(normalMatrix * tangent.xyz);

        if (isAttributeExists(prim.bitangent)) {
            outBitangent = normalize(normalMatrix * getBitangent(prim, gl_VertexIndex));
        } else {
            outBitangent = normalize(cross(outNormal, outTangent) * (tangent.w != 0.0 ? tangent.w : 1.0));
        }
    }

    gl_Position = camera.proj * camera.view * worldPos;
}
