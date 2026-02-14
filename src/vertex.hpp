#pragma once

#include <cstddef>

#include "vulkan/vulkan.hpp"
namespace lvk {
struct Vertex {
  glm::vec2 position{};
  glm::vec3 color{1.0F};
  glm::vec2 uv{};
};

constexpr auto kVertexAttributesV = std::array{
    vk::VertexInputAttributeDescription2EXT{0, 0, vk::Format::eR32G32Sfloat,
                                            offsetof(Vertex, position)},
    vk::VertexInputAttributeDescription2EXT{1, 0, vk::Format::eR32G32B32Sfloat,
                                            offsetof(Vertex, color)},
    vk::VertexInputAttributeDescription2EXT{2, 0, vk::Format::eR32G32Sfloat,
                                            offsetof(Vertex, uv)},
};

constexpr auto kVertexBindingsV = std::array{
    vk::VertexInputBindingDescription2EXT{0, sizeof(Vertex),
                                          vk::VertexInputRate::eVertex, 1},
};
};  // namespace lvk
