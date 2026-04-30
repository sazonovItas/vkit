#pragma once

#include <glm/glm.hpp>
#include <numbers>
#include <optional>
#include <string_view>

#include "vkit/item/item.hpp"
#include "vkit/scene/node_attachment.hpp"
#include "vkit/scene/transform.hpp"

namespace vkit::scene {

enum class CameraType { kPerspective, kOrthographic };

struct PerspectiveParams {
  float fovY{std::numbers::pi / 4.0F};
  std::optional<float> aspectRatio;
  float zNear{0.01F};
  std::optional<float> zFar;
};

struct OrthographicParams {
  float xMag{1.0F};
  float yMag{1.0F};
  float zNear{0.01F};
  float zFar{100.0F};
};

class Camera : public Item<Camera>, public NodeAttachment {
 public:
  explicit Camera(std::string_view name = "Camera") : Item{name} {
    updateProjection();
    updateView();
  }

  void setType(CameraType newType) {
    type_ = newType;
    updateProjection();
  }

  void setPerspective(const PerspectiveParams& params) {
    perspective_ = params;
    updateProjection();
  }

  void setOrthographic(const OrthographicParams& params) {
    orthographic_ = params;
    updateProjection();
  }

  void setFallbackAspectRatio(float aspect) {
    fallbackAspectRatio_ = aspect;
    updateProjection();
  }

  void setAspectRatio(float aspect) {
    perspective_.aspectRatio = aspect;
    updateProjection();
  }

  void setPosition(const glm::vec3& newPosition) {
    position_ = newPosition;
    updateView();
  }

  void setTarget(const glm::vec3& newTarget) {
    target_ = newTarget;
    updateView();
  }

  void setUpVector(const glm::vec3& newUp) {
    up_ = newUp;
    updateView();
  }

  void lookAt(const glm::vec3& newPosition, const glm::vec3& newTarget,
              const glm::vec3& newUp = glm::vec3(0.0F, 1.0F, 0.0F)) {
    position_ = newPosition;
    target_ = newTarget;
    up_ = newUp;
    updateView();
  }

  void setViewTransform(const Transform& transform) {
    viewTransform_ = transform;
  }

  [[nodiscard]] auto getProjectionMatrix() const -> glm::mat4 {
    auto proj = projectionTransform_.getMatrix();
    proj[1][1] *= -1.0F;
    return proj;
  }

  [[nodiscard]] auto getViewMatrix() const -> glm::mat4 {
    return viewTransform_.getMatrix();
  }

  [[nodiscard]] auto getViewProjectionMatrix() const -> glm::mat4 {
    return getProjectionMatrix() * getViewMatrix();
  }

  [[nodiscard]] auto getProjection() const -> const Transform& {
    return projectionTransform_;
  }

  [[nodiscard]] auto getView() const -> const Transform& {
    return viewTransform_;
  }

  [[nodiscard]] auto getPosition() const -> glm::vec3 { return position_; }
  [[nodiscard]] auto getTarget() const -> glm::vec3 { return target_; }
  [[nodiscard]] auto getUpVector() const -> glm::vec3 { return up_; }
  [[nodiscard]] auto getCameraType() const -> CameraType { return type_; }

  [[nodiscard]] auto getPerspectiveParams() const -> const PerspectiveParams& {
    return perspective_;
  }

  [[nodiscard]] auto getOrthographicParams() const
      -> const OrthographicParams& {
    return orthographic_;
  }

 private:
  CameraType type_{CameraType::kPerspective};
  PerspectiveParams perspective_;
  OrthographicParams orthographic_;

  glm::vec3 position_{0.0F, 0.0F, 0.0F};
  glm::vec3 target_{0.0F, 0.0F, -1.0F};
  glm::vec3 up_{0.0F, 1.0F, 0.0F};

  float fallbackAspectRatio_{16.0F / 9.0F};

  Transform projectionTransform_;
  Transform viewTransform_;

  void updateProjection() {
    if (type_ == CameraType::kPerspective) {
      const float aspect =
          perspective_.aspectRatio.value_or(fallbackAspectRatio_);
      const float z_far = perspective_.zFar.value_or(10000.0F);
      projectionTransform_.setPerspective(perspective_.fovY, aspect,
                                          perspective_.zNear, z_far);
    } else {
      projectionTransform_.setOrthographic(
          -orthographic_.xMag, orthographic_.xMag, -orthographic_.yMag,
          orthographic_.yMag, orthographic_.zNear, orthographic_.zFar);
    }
  }

  void updateView() { viewTransform_.setView(position_, target_, up_); }
};

};  // namespace vkit::scene
