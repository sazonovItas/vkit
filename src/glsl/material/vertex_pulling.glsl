#ifndef VERTEX_PULLING_GLSL
#define VERTEX_PULLING_GLSL

#include "types.glsl"

// NOTE: formats fully depends on the Vulkan VK_FORMAT
#define FORMAT_R32G32B32_SFLOAT   106u
#define FORMAT_R32G32B32A32_SFLOAT 109u

#define FORMAT_R32G32_SFLOAT      103u
#define FORMAT_R32_SFLOAT         100u

#define FORMAT_R16G16B16A16_SFLOAT 89u
#define FORMAT_R16G16_SFLOAT      79u

#define FORMAT_R8G8B8A8_UNORM     37u
#define FORMAT_R8G8B8A8_SNORM     38u

#define FORMAT_R8G8B8A8_UINT      41u


layout(std430, buffer_reference, buffer_reference_align = 4)
readonly buffer UIntRef { uint data; };

layout(std430, buffer_reference, buffer_reference_align = 4)
readonly buffer Vec2Ref { vec2 data; };

layout(std430, buffer_reference, buffer_reference_align = 4)
readonly buffer Vec3Ref { vec3 data; };

layout(std430, buffer_reference, buffer_reference_align = 4)
readonly buffer Vec4Ref { vec4 data; };

layout(std430, buffer_reference, buffer_reference_align = 4)
readonly buffer IVec2Ref { ivec2 data; };

layout(std430, buffer_reference, buffer_reference_align = 4)
readonly buffer IVec3Ref { ivec3 data; };

layout(std430, buffer_reference, buffer_reference_align = 4)
readonly buffer IVec4Ref { ivec4 data; };

layout(std430, buffer_reference, buffer_reference_align = 4)
readonly buffer UVec4Ref { uvec4 data; };

vec2 load_vec2(Attribute a, uvec2 addr) {
  switch (a.format)
  {
    case FORMAT_R32G32_SFLOAT:
      return Vec2Ref(addr).data;

    case FORMAT_R16G16_SFLOAT:
      return Vec2Ref(addr).data;

    case FORMAT_R8G8B8A8_UNORM:
      return unpackUnorm4x8(UIntRef(addr).data).xy;

    case FORMAT_R8G8B8A8_SNORM:
      return unpackSnorm4x8(UIntRef(addr).data).xy;
  }

  return vec2(0.0);
}

vec3 load_vec3(Attribute a, uvec2 addr) {
  switch (a.format)
  {
    case FORMAT_R32G32B32_SFLOAT:
      return Vec3Ref(addr).data;

    case FORMAT_R16G16B16A16_SFLOAT:
      return Vec3Ref(addr).data;

    case FORMAT_R8G8B8A8_UNORM:
      return unpackUnorm4x8(UIntRef(addr).data).xyz;

    case FORMAT_R8G8B8A8_SNORM:
      return unpackSnorm4x8(UIntRef(addr).data).xyz;
  }

  return vec3(0.0);
}

vec4 load_vec4(Attribute a, uvec2 addr) {
  switch (a.format)
  {
    case FORMAT_R32G32B32A32_SFLOAT:
      return Vec4Ref(addr).data;

    case FORMAT_R8G8B8A8_UNORM:
      return unpackUnorm4x8(UIntRef(addr).data);

    case FORMAT_R8G8B8A8_SNORM:
      return unpackSnorm4x8(UIntRef(addr).data);
  }

  return vec4(0.0);
}

uvec4 load_uvec4(Attribute a, uvec2 addr) {
  switch (a.format)
  {
    case FORMAT_R8G8B8A8_UINT:
      return UVec4Ref(addr).data;
  }

  return uvec4(0);
}

vec3 getPosition(Primitive prim, uint idx) {
  Attribute a = prim.position;
  return load_vec3(a, getFetchAddress(a, idx));
}

vec3 getNormal(Primitive prim, uint idx) {
  Attribute a = prim.normal;
  return load_vec3(a, getFetchAddress(a, idx));
}

vec3 getTangent(Primitive prim, uint idx) {
  Attribute a = prim.tangent;
  return load_vec3(a, getFetchAddress(a, idx));
}

vec3 getBitangent(Primitive prim, uint idx) {
  Attribute a = prim.bitangent;
  return load_vec3(a, getFetchAddress(a, idx));
}

vec2 getTexcoord(Primitive prim, uint texcoordIdx, uint idx) {
  Attribute a = prim.texcoords[texcoordIdx];
  return load_vec2(a, getFetchAddress(a, idx));
}

vec4 getColor(Primitive prim, uint idx) {
  Attribute a = prim.color;
  return load_vec4(a, getFetchAddress(a, idx));
}

uvec4 getJoints(Primitive prim, uint idx) {
  Attribute a = prim.jointIndices;
  return load_uvec4(a, getFetchAddress(a, idx));
}

vec4 getWeights(Primitive prim, uint idx) {
  Attribute a = prim.jointWeights;
  return load_vec4(a, getFetchAddress(a, idx));
}

#endif // VERTEX_PULLING_GLSL
