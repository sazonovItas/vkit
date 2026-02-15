#pragma once

namespace lvk {
struct Transform {
  glm::vec3 position{};
  glm::vec3 rotation{};
  glm::vec3 scale{1.0F};

  [[nodiscard]] auto model_matrix() const -> glm::mat4;
  [[nodiscard]] auto view_matrix() const -> glm::mat4;
};
};  // namespace lvk
