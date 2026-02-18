#extension GL_EXT_buffer_reference : require
#extension GL_EXT_scalar_block_layout: require

#include "vertex.glsl"

layout (push_constant, scalar) uniform constants
{
    VertexBuffer vertex_buffer;
} pcs;
