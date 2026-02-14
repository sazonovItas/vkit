#include "transform.hpp"

#include "glm/ext/matrix_transform.hpp"
#include "glm/trigonometric.hpp"

namespace lvk {
namespace {
struct Matricies {
  glm::mat4 translation;
  glm::mat4 orientation;
  glm::mat4 scale;
};

[[nodiscard]] auto to_matricies(glm::vec2 const position, float rotation,
                                glm::vec2 const scale) -> Matricies {
  static constexpr auto kMatV = glm::identity<glm::mat4>();
  static constexpr auto kAxisV = glm::vec3{0.0F, 0.0F, 1.0F};
  return Matricies{
      .translation = glm::translate(kMatV, glm::vec3{position, 0.0F}),
      .orientation = glm::rotate(kMatV, glm::radians(rotation), kAxisV),
      .scale = glm::scale(kMatV, glm::vec3{scale, 1.0F}),
  };
}
};  // namespace

auto Transform::model_matrix() const -> glm::mat4 {
  auto const [t, r, s] = to_matricies(position, rotation, scale);
  return t * r * s;
}

auto Transform::view_matrix() const -> glm::mat4 {
  auto const [t, r, s] = to_matricies(-position, -rotation, scale);
  return r * t * s;
}
};  // namespace lvk
