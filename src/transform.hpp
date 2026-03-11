#pragma once

#include "glm/gtc/quaternion.hpp"

namespace lvk {
struct Transform {
  glm::vec3 position{};
  glm::quat rotation{1.0F, 0.0F, 0.0F, 0.0F};
  glm::vec3 scale{1.0F};

  [[nodiscard]] auto modelMatrix() const -> glm::mat4;
  [[nodiscard]] auto viewMatrix() const -> glm::mat4;
};
};  // namespace lvk
