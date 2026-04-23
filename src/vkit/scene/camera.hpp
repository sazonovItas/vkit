#pragma once

#include <numbers>
#include <optional>

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

class Camera : public NodeAttachment {
 public:
  explicit Camera(std::string_view name = "Camera") : NodeAttachment(name) {}

  [[nodiscard]] auto getType() const -> AttachmentType override {
    return AttachmentType::kCamera;
  }

  CameraType type{CameraType::kPerspective};
  PerspectiveParams perspective;
  OrthographicParams orthographic;

  [[nodiscard]] auto getProjection(float fallbackAspectRatio = 16.0F /
                                                               9.0F) const
      -> Transform {
    Transform proj;

    if (type == CameraType::kPerspective) {
      float aspect = perspective.aspectRatio.value_or(fallbackAspectRatio);
      float z_far = perspective.zFar.value_or(10000.0F);
      proj.setPerspective(perspective.fovY, aspect, perspective.zNear, z_far);
    } else {
      proj.setOrthographic(-orthographic.xMag, orthographic.xMag,
                           -orthographic.yMag, orthographic.yMag,
                           orthographic.zNear, orthographic.zFar);
    }

    return proj;
  }
};

};  // namespace vkit::scene
