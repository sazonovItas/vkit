#include "transform.hpp"

#include "glm/ext/matrix_transform.hpp"
#include "glm/fwd.hpp"

namespace vkit {
namespace {
struct Matricies {
  glm::mat4 translation;
  glm::mat4 orientation;
  glm::mat4 scale;
};

[[nodiscard]] auto toMatricies(glm::vec3 const position, glm::quat rotation,
                               glm::vec3 const scale) -> Matricies {
  static constexpr auto kMatV = glm::identity<glm::mat4>();
  return Matricies{
      .translation = glm::translate(kMatV, position),
      .orientation = glm::mat4{rotation},
      .scale = glm::scale(kMatV, scale),
  };
}
};  // namespace

auto Transform::modelMatrix() const -> glm::mat4 {
  auto const [t, r, s] = toMatricies(position, rotation, scale);
  return t * r * s;
}

auto Transform::viewMatrix() const -> glm::mat4 {
  auto const [t, r, s] = toMatricies(-position, -rotation, scale);
  return r * t * s;
}
};  // namespace vkit
