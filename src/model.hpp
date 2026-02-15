#pragma once

#include <cstddef>

#include "vma.hpp"
#include "vulkan/vulkan.hpp"

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
  glm::vec3 color{1.0F};
  glm::vec2 uv{};
  glm::vec3 normal{};
};

struct Primitive {
  DrawCommand draw;

  vma::Buffer vertex_buffer;
  vma::Buffer index_buffer;
};

struct Mesh {
  std::vector<Primitive> primitives;
};

constexpr auto kVertexAttributesV = std::array{
    vk::VertexInputAttributeDescription2EXT{0, 0, vk::Format::eR32G32B32Sfloat,
                                            offsetof(Vertex, position)},
    vk::VertexInputAttributeDescription2EXT{1, 0, vk::Format::eR32G32B32Sfloat,
                                            offsetof(Vertex, color)},
    vk::VertexInputAttributeDescription2EXT{2, 0, vk::Format::eR32G32Sfloat,
                                            offsetof(Vertex, uv)},
    vk::VertexInputAttributeDescription2EXT{3, 0, vk::Format::eR32G32B32Sfloat,
                                            offsetof(Vertex, normal)},
};

constexpr auto kVertexBindingsV = std::array{
    vk::VertexInputBindingDescription2EXT{0, sizeof(Vertex),
                                          vk::VertexInputRate::eVertex, 1},
};
};  // namespace lvk
