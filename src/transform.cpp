#include "transform.hpp"

#include "glm/ext/matrix_transform.hpp"
#include "glm/fwd.hpp"
#include "glm/trigonometric.hpp"

namespace lvk {
namespace {
struct Matricies {
  glm::mat4 translation;
  glm::mat4 orientation;
  glm::mat4 scale;
};

[[nodiscard]] auto to_matricies(glm::vec3 const position, glm::vec3 rotation,
                                glm::vec3 const scale) -> Matricies {
  static constexpr auto kMatV = glm::identity<glm::mat4>();

  static constexpr auto kXAxe = glm::vec3{1.0F, 0.0F, 0.0F};
  static constexpr auto kYAxe = glm::vec3{0.0F, 1.0F, 0.0F};
  static constexpr auto kZAxe = glm::vec3{0.0F, 0.0F, 1.0F};
  return Matricies{
      .translation = glm::translate(kMatV, position),
      .orientation = glm::rotate(kMatV, glm::radians(rotation.x), kXAxe) *
                     glm::rotate(kMatV, glm::radians(rotation.y), kYAxe) *
                     glm::rotate(kMatV, glm::radians(rotation.z), kZAxe),
      .scale = glm::scale(kMatV, scale),
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
