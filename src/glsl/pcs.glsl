#ifndef PCS_GLSL
#define PCS_GLSL

#extension GL_EXT_buffer_reference : require
#extension GL_EXT_scalar_block_layout: require

#include "vertex.glsl"

layout (push_constant, scalar) uniform PushConstants
{
  int materialIdx;
  VertexBuffer vertices;
} pcs;

#endif // PCS_GLSL
