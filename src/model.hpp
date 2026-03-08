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
  glm::vec4 baseColorFactor;
  glm::vec4 emissiveFactor;
  glm::vec4 diffuseFactor;
  glm::vec4 specularFactor;

  std::int32_t baseColorTextureIdx;
  std::int32_t metallicRoughnessTextureIdx;
  std::int32_t normalTextureIdx;
  std::int32_t occlusionTextureIdx;

  std::int32_t emissiveTextureIdx;
  std::int32_t _padding0[3];

  float metallicFactor;
  float roughnessFactor;
  float alphaMask;
  float alphaMaskCutoff;

  float emissiveStrength;
  float _padding1[3];
};

struct Vertex {
  glm::vec3 position{};
  float uv_x{};
  glm::vec3 normal{};
  float uv_y{};
  glm::vec4 tangent{};
};
};  // namespace lvk
