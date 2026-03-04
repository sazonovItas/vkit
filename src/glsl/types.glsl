#ifndef TYPES_GLSL
#define TYPES_GLSL

struct Material {
  int16_t base_color_tex_idx;
  int16_t metallic_roughness_tex_idx;
  int16_t normal_tex_idx;
  int16_t emissive_tex_idx;
  int16_t occlusion_tex_idx;
  int16_t _padding0[3];
  vec4 base_color_factor;
  float metallic_factor;
  float roughness_factor;
  float _padding1[2];
};

layout (buffer_reference) readonly buffer Matrices { mat4 data[]; };

struct Node {
  mat4 world_transform;
};

struct Accessor {
  uvec2 buffer_address;
  uint component_type;
  uint stride;
};

uvec add64(uvec2 lhs, uint rhs) {
  uint carry;
  uint lo = uaddCarry(lhs.x, rhs, carry);
  uint hi = lhs.y + carry;
  return uvec2(lo, hi);
}

uvec2 get_fetch_address(Accessor accessor, uint index) {
  return add64(accessor.buffer_address, accessor.stride * index);
}

layout (std430, buffer_reference, buffer_reference_align = 16) readonly buffer Accessors { Accessor data[]; };

struct Primitive {
  Accessor position;
  Accessor normal;
  Accessor tangent;
  Accessor texture[4];
  int material_idx;
  uint _padding;
};

#endif
