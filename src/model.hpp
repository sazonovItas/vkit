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
  std::int32_t materialIdx{-1};
  vku::DeviceBuffer vertexBuffer;
  vku::DeviceBuffer indexBuffer;
};

struct Mesh {
  std::vector<Primitive> primitives;
};

struct Material {
  std::int16_t baseColorTexIdx{-1};
  std::int16_t metallicRoughnessTexIdx{-1};
  std::int16_t normalTexIdx{-1};
  std::int16_t emissiveTexIdx{-1};
  std::int16_t occlusionTexIdx{-1};
  std::int16_t _padding0[3];
  glm::vec4 baseColorFactor{1.0, 1.0, 1.0, 1.0};
  glm::vec4 emmisiveFactor;
  float metallicFactor;
  float roughnessFactor;
  float _padding1[2];
};
};  // namespace lvk
