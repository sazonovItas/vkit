#version 450

layout(location = 0) out vec3 outWorldPos;
layout(location = 1) out vec2 outUV;
layout(location = 2) out vec3 outNormal;
layout(location = 3) out vec3 outTangent;
layout(location = 4) out vec3 outBitangent;

#include "common/primitive_types.glsl"
#include "common/primitive_vertex_pulling.glsl"

layout(set = 0, binding = 0) uniform Camera { mat4 view; mat4 proj; vec3 position; } camera;

layout(std430, set = 3, binding = 0) readonly buffer PrimitiveBlock { 
  Primitive data[]; 
} primitives;

layout(std430, set = 3, binding = 1) readonly buffer JointBlock {
  mat4 joints[];
} jointData;

layout(push_constant) uniform PushConstants {
  mat4 model;
  uint primIndex;
  uint materialIndex;
} pcs;

void main() {
  Primitive prim = primitives.data[pcs.primIndex];

  vec3 pos    = getPosition(prim, gl_VertexIndex);
  vec3 normal = getNormal(prim, gl_VertexIndex);

  vec2 uv = vec2(0.0);
  if (isAttributeExists(prim.texcoords[0])) {
    uv = getTexcoord(prim, 0, gl_VertexIndex);
  }

  mat4 skinMatrix = mat4(1.0);
  if (isAttributeExists(prim.jointIndices) && isAttributeExists(prim.jointWeights)) {
    uvec4 j = getJoints(prim, gl_VertexIndex);
    vec4 w  = getWeights(prim, gl_VertexIndex);

    skinMatrix = 
        w.x * jointData.joints[j.x] +
        w.y * jointData.joints[j.y] +
        w.z * jointData.joints[j.z] +
        w.w * jointData.joints[j.w];
  }

  vec4 worldPos = pcs.model * skinMatrix * vec4(pos, 1.0);
  mat3 normalMatrix = transpose(inverse(mat3(pcs.model * skinMatrix)));

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
