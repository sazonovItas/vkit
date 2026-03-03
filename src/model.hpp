#pragma once

#include "vku/buffers/device_buffer.hpp"

namespace lvk {
struct DrawCommand {
  std::uint32_t count{};
  std::uint32_t instance_count{};
  std::uint32_t first_index{};
  std::uint32_t vertex_offset{};
  std::uint32_t first_instance{};
};

struct Vertex {
  glm::vec3 position{};
  float uv_x{};
  glm::vec3 normal{};
  float uv_y{};
  glm::vec4 tangent{};
};

struct Primitive {
  DrawCommand draw{};
  vku::DeviceBuffer vertex_buffer;
  vku::DeviceBuffer index_buffer;
};

struct Mesh {
  std::vector<Primitive> primitives;
};

struct Material {
  glm::vec4 base_color{};
  glm::vec4 metallic_roughness_emissive{};
  std::uint32_t diffuse_tex{};
  std::uint32_t normal_tex{};
  std::uint32_t metallic_roughness_tex{};
  std::uint32_t emissive_tex{};
};
};  // namespace lvk
