#ifndef VERTEX_GLSL
#define VERTEX_GLSL

#extension GL_EXT_buffer_reference : require

struct Vertex {
  vec3 position;
  float uvX;
  vec3 normal;
  float uvY;
  vec4 tangent;
};

layout (buffer_reference, std430) readonly buffer VertexBuffer {
	Vertex data[];
};

#endif // VERTEX_GLSL
