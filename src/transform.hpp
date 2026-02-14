#pragma once

namespace lvk {
struct Transform {
  glm::vec2 position{};
  float rotation{};
  glm::vec2 scale{1.0F};

  [[nodiscard]] auto model_matrix() const -> glm::mat4;
  [[nodiscard]] auto view_matrix() const -> glm::mat4;
};
};  // namespace lvk
