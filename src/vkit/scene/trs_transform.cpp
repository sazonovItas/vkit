#include "vkit/scene/trs_transform.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/quaternion.hpp>

namespace vkit::scene {

using glm::mat4;
using glm::vec3;

TrsTransform::TrsTransform(const mat4 m) : Transform{m} { decompose(); }

TrsTransform::TrsTransform(const mat4 matrix, const mat4 inverse_matrix)
    : Transform{matrix, inverse_matrix} {
  decompose();
}

TrsTransform::TrsTransform(const vec3 translation) : translation_{translation} {
  compose();
}

TrsTransform::TrsTransform(const glm::quat rotation) : rotation_{rotation} {
  compose();
}

TrsTransform::TrsTransform(const vec3 translation, glm::quat rotation)
    : translation_{translation}, rotation_{rotation} {
  compose();
}

TrsTransform::TrsTransform(const vec3 translation, glm::quat rotation,
                           vec3 scale)
    : translation_{translation}, rotation_{rotation}, scale_{scale} {
  compose();
}

TrsTransform::TrsTransform(const vec3 translation, glm::quat rotation,
                           vec3 scale, vec3 skew)
    : translation_{translation},
      rotation_{rotation},
      scale_{scale},
      skew_{skew} {
  compose();
}

void TrsTransform::compose() {
  mat4 t = glm::translate(mat4{1.0F}, translation_);
  mat4 r = glm::toMat4(rotation_);
  mat4 s = glm::scale(mat4{1.0F}, scale_);

  mat4 k{1.0F};
  k[1][0] = skew_.x;
  k[2][0] = skew_.y;
  k[2][1] = skew_.z;

  matrix_ = t * r * k * s;
  inverseMatrix_ = glm::inverse(matrix_);
}

void TrsTransform::decompose() {
  glm::vec4 perspective;
  glm::decompose(matrix_, scale_, rotation_, translation_, skew_, perspective);
}

void TrsTransform::translateBy(vec3 t) {
  translation_ += t;
  compose();
}

void TrsTransform::rotateBy(glm::quat r) {
  rotation_ = r * rotation_;
  compose();
}

void TrsTransform::scaleBy(vec3 s) {
  scale_ *= s;
  compose();
}

void TrsTransform::setTranslation(vec3 t) {
  translation_ = t;
  compose();
}

void TrsTransform::setRotation(glm::quat r) {
  rotation_ = r;
  compose();
}

void TrsTransform::setScale(vec3 s) {
  scale_ = s;
  compose();
}

void TrsTransform::setSkew(vec3 s) {
  skew_ = s;
  compose();
}

void TrsTransform::set_translation_and_rotation(vec3 t, glm::quat r) {
  translation_ = t;
  rotation_ = r;
  compose();
}

void TrsTransform::set_trs(vec3 t, glm::quat r, vec3 s) {
  translation_ = t;
  rotation_ = r;
  scale_ = s;
  compose();
}

void TrsTransform::set(vec3 t, glm::quat r, vec3 s, vec3 skew) {
  translation_ = t;
  rotation_ = r;
  scale_ = s;
  skew_ = skew;
  compose();
}

void TrsTransform::set(const mat4& matrix) {
  matrix_ = matrix;
  inverseMatrix_ = glm::inverse(matrix);
  decompose();
}

void TrsTransform::set(const mat4& matrix, const mat4& inverse_matrix) {
  matrix_ = matrix;
  inverseMatrix_ = inverse_matrix;
  decompose();
}

auto TrsTransform::inverse(const TrsTransform& t) -> TrsTransform {
  return TrsTransform{t.getInverseMatrix(), t.getMatrix()};
}

auto translate(const TrsTransform& t, vec3 tr) -> TrsTransform {
  return TrsTransform{t.getTranslation() + tr, t.getRotation(), t.getScale(),
                      t.getSkew()};
}

auto rotate(const TrsTransform& t, glm::quat r) -> TrsTransform {
  return TrsTransform{t.getTranslation(), r * t.getRotation(), t.getScale(),
                      t.getSkew()};
}

auto scale(const TrsTransform& t, vec3 s) -> TrsTransform {
  return TrsTransform{t.getTranslation(), t.getRotation(), t.getScale() * s,
                      t.getSkew()};
}

auto skew(const TrsTransform& t, vec3 sk) -> TrsTransform {
  return TrsTransform{t.getTranslation(), t.getRotation(), t.getScale(),
                      t.getSkew() + sk};
}

auto operator*(const TrsTransform& lhs, const TrsTransform& rhs)
    -> TrsTransform {
  return TrsTransform{lhs.getMatrix() * rhs.getMatrix(),
                      rhs.getInverseMatrix() * lhs.getInverseMatrix()};
}

auto operator==(const TrsTransform& lhs, const TrsTransform& rhs) -> bool {
  return lhs.getMatrix() == rhs.getMatrix() &&
         lhs.getInverseMatrix() == rhs.getInverseMatrix();
}

auto operator!=(const TrsTransform& lhs, const TrsTransform& rhs) -> bool {
  return !(lhs == rhs);
}

};  // namespace vkit::scene
