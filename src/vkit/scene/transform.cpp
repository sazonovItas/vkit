#include "vkit/scene/transform.hpp"

#include "glm/ext/matrix_clip_space.hpp"
#include "glm/ext/matrix_transform.hpp"

namespace vkit::scene {

using glm::mat4;
using glm::vec3;

Transform::Transform(const mat4 m)
    : matrix_{m}, inverseMatrix_{glm::inverse(m)} {}

Transform::Transform(const mat4 matrix, const mat4 inverseMatrix)
    : matrix_{matrix}, inverseMatrix_{glm::inverse(inverseMatrix)} {}

auto Transform::inverse(const Transform& transform) -> Transform {
  return Transform{
      transform.getMatrix(),
      transform.getInverseMatrix(),
  };
}

void Transform::setOrthographic(const float left, const float right,
                                const float bottom, const float top,
                                const float zNear, const float zFar) {
  matrix_ = glm::ortho(left, right, bottom, top, zNear, zFar);
  inverseMatrix_ = glm::inverse(matrix_);
}

void Transform::setPerspective(float fovY, float aspectRatio, float zNear,
                               float zFar) {
  matrix_ = glm::perspective(fovY, aspectRatio, zNear, zFar);
  inverseMatrix_ = glm::inverse(matrix_);
}

void Transform::setView(glm::vec3 position, glm::vec3 target, glm::vec3 up) {
  matrix_ = glm::lookAt(position, target, up);
  inverseMatrix_ = glm::inverse(matrix_);
}

Transform operator*(const Transform& lhs, const Transform& rhs) {
  return Transform{lhs.getMatrix() * rhs.getMatrix()};
}

};  // namespace vkit::scene
