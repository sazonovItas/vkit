#ifndef SKYBOX_PCS_GLSL
#define SKYBOX_PCS_GLSL

#extension GL_EXT_buffer_reference : require
#extension GL_EXT_scalar_block_layout: require

layout (push_constant, scalar) uniform PushConstants
{
  vec4 envBaseColor;
  int envMapIdx;
} pcs;

#endif // SKYBOX_PCS_GLSL
