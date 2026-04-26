#ifndef VERTEX_PULLING_GLSL
#define VERTEX_PULLING_GLSL

#extension GL_EXT_buffer_reference : require
#extension GL_EXT_buffer_reference_uvec2 : require

#include "primitive_types.glsl"

// Float32 (4 bytes)
#define ATTR_FORMAT_SCALAR_FLOAT32 1u
#define ATTR_FORMAT_VEC2_FLOAT32   2u
#define ATTR_FORMAT_VEC3_FLOAT32   3u
#define ATTR_FORMAT_VEC4_FLOAT32   4u

// UInt32 (4 bytes)
#define ATTR_FORMAT_SCALAR_UINT32  8u
#define ATTR_FORMAT_VEC2_UINT32    9u
#define ATTR_FORMAT_VEC3_UINT32    10u
#define ATTR_FORMAT_VEC4_UINT32    11u

// UInt16 (2 bytes)
#define ATTR_FORMAT_SCALAR_UINT16  15u
#define ATTR_FORMAT_VEC2_UINT16    16u
#define ATTR_FORMAT_VEC3_UINT16    17u
#define ATTR_FORMAT_VEC4_UINT16    18u

// UInt8 (1 byte)
#define ATTR_FORMAT_SCALAR_UINT8   29u
#define ATTR_FORMAT_VEC2_UINT8     30u
#define ATTR_FORMAT_VEC3_UINT8     31u
#define ATTR_FORMAT_VEC4_UINT8     32u

// Int8 (1 byte)
#define ATTR_FORMAT_SCALAR_INT8    36u
#define ATTR_FORMAT_VEC2_INT8      37u
#define ATTR_FORMAT_VEC3_INT8      38u
#define ATTR_FORMAT_VEC4_INT8      39u


layout(std430, buffer_reference, buffer_reference_align = 4) readonly buffer UIntRef { uint data; };
layout(std430, buffer_reference, buffer_reference_align = 4) readonly buffer Vec2Ref { vec2 data; };
layout(std430, buffer_reference, buffer_reference_align = 4) readonly buffer Vec3Ref { vec3 data; };
layout(std430, buffer_reference, buffer_reference_align = 4) readonly buffer Vec4Ref { vec4 data; };
layout(std430, buffer_reference, buffer_reference_align = 4) readonly buffer UVec2Ref { uvec2 data; };
layout(std430, buffer_reference, buffer_reference_align = 4) readonly buffer UVec4Ref { uvec4 data; };


uvec2 add64(uvec2 a, uint b)
{
  uint lo = a.x + b;
  uint carry = (lo < a.x) ? 1u : 0u;
  return uvec2(lo, a.y + carry);
}

uvec2 add64(uvec2 a, uvec2 b)
{
  uint lo = a.x + b.x;
  uint carry = (lo < a.x) ? 1u : 0u;
  return uvec2(lo, a.y + b.y + carry);
}

uvec2 getFetchAddress(Attribute attr, uint vertexIndex)
{
  uint byteOffset = attr.offset + attr.stride * vertexIndex;
  return add64(attr.addr, byteOffset);
}


vec2 load_vec2(Attribute a, uvec2 addr) {
  switch (a.format)
  {
    case ATTR_FORMAT_VEC2_FLOAT32:
      return Vec2Ref(addr).data;

    case ATTR_FORMAT_VEC2_UINT16:
      return unpackUnorm2x16(UIntRef(addr).data);

    case ATTR_FORMAT_VEC2_UINT8:
    case ATTR_FORMAT_VEC4_UINT8:
      return unpackUnorm4x8(UIntRef(addr).data).xy;

    case ATTR_FORMAT_VEC2_INT8:
    case ATTR_FORMAT_VEC4_INT8:
      return unpackSnorm4x8(UIntRef(addr).data).xy;
  }

  return vec2(0.0);
}

vec3 load_vec3(Attribute a, uvec2 addr) {
  switch (a.format)
  {
    case ATTR_FORMAT_VEC3_FLOAT32:
      return Vec3Ref(addr).data;

    case ATTR_FORMAT_VEC3_UINT8:
    case ATTR_FORMAT_VEC4_UINT8:
      return unpackUnorm4x8(UIntRef(addr).data).xyz;

    case ATTR_FORMAT_VEC3_INT8:
    case ATTR_FORMAT_VEC4_INT8:
      return unpackSnorm4x8(UIntRef(addr).data).xyz;
  }

  return vec3(0.0);
}

vec4 load_vec4(Attribute a, uvec2 addr) {
  switch (a.format)
  {
    case ATTR_FORMAT_VEC4_FLOAT32:
      return Vec4Ref(addr).data;

    case ATTR_FORMAT_VEC4_UINT8:
      return unpackUnorm4x8(UIntRef(addr).data);

    case ATTR_FORMAT_VEC4_INT8:
      return unpackSnorm4x8(UIntRef(addr).data);
  }

  return vec4(0.0);
}

uvec4 load_uvec4(Attribute a, uvec2 addr) {
  switch (a.format)
  {
    case ATTR_FORMAT_VEC4_UINT32:
      return UVec4Ref(addr).data;
    
    case ATTR_FORMAT_VEC4_UINT16: {
      uvec2 raw = UVec2Ref(addr).data;
      return uvec4(
        raw.x & 0xFFFFu, raw.x >> 16u, 
        raw.y & 0xFFFFu, raw.y >> 16u
      );
    }

    case ATTR_FORMAT_VEC4_UINT8: {
      uint raw = UIntRef(addr).data;
      return uvec4(
        raw & 0xFFu, 
        (raw >> 8u) & 0xFFu, 
        (raw >> 16u) & 0xFFu, 
        (raw >> 24u) & 0xFFu
      );
    }
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

vec4 getTangent(Primitive prim, uint idx) {
  Attribute a = prim.tangent;
  return load_vec4(a, getFetchAddress(a, idx));
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
