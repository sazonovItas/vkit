#pragma once

namespace vkit::scene {

class Transform {
 public:
  Transform() = default;
  explicit Transform(glm::mat4 m);
  Transform(glm::mat4 matrix, glm::mat4 inverseMatrix);

  [[nodiscard]] auto getMatrix() const -> const glm::mat4& { return matrix_; }
  [[nodiscard]] auto getInverseMatrix() const -> const glm::mat4& {
    return inverseMatrix_;
  }

  [[nodiscard]] static auto inverse(const Transform& transform) -> Transform;

  void setOrthographic(float left, float right, float bottom, float top,
                       float zNear, float zFar);
  void setPerspective(float fovY, float aspectRatio, float zNear, float zFar);

  void setView(glm::vec3 position, glm::vec3 target, glm::vec3 up);

 protected:
  glm::mat4 matrix_{1.0F};
  glm::mat4 inverseMatrix_{1.0F};
};

Transform operator*(const Transform& lhs, const Transform& rhs);

};  // namespace vkit::scene
