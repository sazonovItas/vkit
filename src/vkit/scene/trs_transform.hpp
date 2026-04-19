#pragma once

#include <glm/gtc/quaternion.hpp>

#include "vkit/scene/transform.hpp"

namespace vkit::scene {

class TrsTransform : public Transform {
 public:
  TrsTransform() = default;
  explicit TrsTransform(glm::mat4 m);
  TrsTransform(glm::mat4 matrix, glm::mat4 inverse_matrix);
  explicit TrsTransform(glm::vec3 translation);
  explicit TrsTransform(glm::quat rotation);
  TrsTransform(glm::vec3 translation, glm::quat rotation);
  TrsTransform(glm::vec3 translation, glm::quat rotation, glm::vec3 scale);
  TrsTransform(glm::vec3 translation, glm::quat rotation, glm::vec3 scale,
               glm::vec3 skew);

  [[nodiscard]] static auto inverse(const TrsTransform& transform)
      -> TrsTransform;

  void translateBy(glm::vec3 translation);
  void rotateBy(glm::quat rotation);
  void scaleBy(glm::vec3 scale);

  void setTranslation(glm::vec3 translation);
  void setRotation(glm::quat rotation);
  void setScale(glm::vec3 scale);
  void setSkew(glm::vec3 skew);
  void set_translation_and_rotation(glm::vec3 translation, glm::quat rotation);
  void set_trs(glm::vec3 translation, glm::quat rotation, glm::vec3 scale);
  void set(glm::vec3 translation, glm::quat rotation, glm::vec3 scale,
           glm::vec3 skew);

  void set(const glm::mat4& matrix);
  void set(const glm::mat4& matrix, const glm::mat4& inverse_matrix);

  [[nodiscard]] auto getScale() const -> glm::vec3 { return scale_; }
  [[nodiscard]] auto getRotation() const -> glm::quat { return rotation_; }
  [[nodiscard]] auto getTranslation() const -> glm::vec3 {
    return translation_;
  }
  [[nodiscard]] auto getSkew() const -> glm::vec3 { return skew_; }

 private:
  void compose();
  void decompose();

  glm::vec3 translation_{0.0F};
  glm::quat rotation_{1.0F, 0.0F, 0.0F, 0.0F};
  glm::vec3 scale_{1.0F};
  glm::vec3 skew_{0.0F};
};

[[nodiscard]] auto translate(const TrsTransform& t, glm::vec3 translation)
    -> TrsTransform;
[[nodiscard]] auto rotate(const TrsTransform& t, glm::quat rotation)
    -> TrsTransform;
[[nodiscard]] auto scale(const TrsTransform& t, glm::vec3 scale)
    -> TrsTransform;
[[nodiscard]] auto skew(const TrsTransform& t, glm::vec3 skew) -> TrsTransform;

auto operator*(const TrsTransform& lhs, const TrsTransform& rhs)
    -> TrsTransform;
auto operator==(const TrsTransform& lhs, const TrsTransform& rhs) -> bool;
auto operator!=(const TrsTransform& lhs, const TrsTransform& rhs) -> bool;

};  // namespace vkit::scene
