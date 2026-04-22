#version 450

#extension GL_EXT_buffer_reference : require

#include "vertex_pulling.glsl"

layout (set = 0, binding = 0) uniform Scene {
} scene;

layout(set = 0, binding = 1) uniform Camera {
    mat4 view;
    mat4 proj;
    vec3 position;
} camera;

layout(push_constant) uniform PushConstants {
    mat4 model;
    Primitive prim;
} pcs;

layout(location = 0) out vec3 outWorldPos;
layout(location = 1) out vec2 outUV;
layout(location = 2) out vec3 outNormal;
layout(location = 3) out vec3 outTangent;
layout(location = 4) out vec3 outBitangent;

void main() {
    vec3 pos = getPosition(pcs.prim, gl_VertexIndex);
    vec3 normal = getNormal(pcs.prim, gl_VertexIndex);

    vec2 uv = vec2(0.0);
    if (isAttributeExisting(pcs.prim.texcoords[0])) {
        uv = getTexcoord(pcs.prim, 0, gl_VertexIndex);
    }

    vec4 worldPos = pcs.model * vec4(pos, 1.0);
    mat3 normalMatrix = transpose(inverse(mat3(pcs.model)));

    outWorldPos = worldPos.xyz;
    outUV = uv;
    outNormal = normalize(normalMatrix * normal);

    outTangent = vec3(0.0);
    outBitangent = vec3(0.0);
    if (isAttributeExisting(pcs.prim.tangent)) {
        outTangent = normalize(normalMatrix * getTangent(pcs.prim, gl_VertexIndex));
        
        if (isAttributeExisting(pcs.prim.bitangent)) {
            outBitangent = normalize(normalMatrix * getBitangent(pcs.prim, gl_VertexIndex));
        }
    }

    gl_Position = camera.proj * camera.view * worldPos;
}
