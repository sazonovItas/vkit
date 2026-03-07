#ifndef MATERIAL_GLSL
#define MATERIAL_GLSL

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

#endif // MATERIAL_GLSL
