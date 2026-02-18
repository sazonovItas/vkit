#pragma once

#include "vma.hpp"

namespace lvk {
struct DrawCommand {
  std::uint32_t count;
  std::uint32_t instance_count;
  std::uint32_t first_index;
  std::uint32_t vertex_offset;
  std::uint32_t first_instance;
};

struct Vertex {
  glm::vec3 position{};
  float uv_x{};
  glm::vec3 normal{};
  float uv_y{};
  glm::vec4 tangent{};
};

struct Primitive {
  DrawCommand draw;

  vma::Buffer vertex_buffer;
  vma::Buffer index_buffer;
};

struct Mesh {
  std::vector<Primitive> primitives;
};
};  // namespace lvk
