#pragma once

#include <cstdint>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <numbers>

#include "vkit/item/item.hpp"
#include "vkit/item/storage_item.hpp"

namespace vkit::scene {

enum class LightType : std::uint32_t {
  kDirectional = 0,
  kPoint,
  kSpot,
};

class Light : public Item<Light>, public StorageItem {
 public:
  using Type = LightType;

  struct alignas(16) Data {
    glm::vec4 positionAndType;
    glm::vec4 colorAndIntensity;
    glm::vec4 directionAndRange;
    glm::vec4 anglesAndPadding;
  };

  static constexpr const char* kCTypeStrings[] = {
      "Directional",
      "Point",
      "Spot",
  };

  explicit Light(std::string_view name = "Light") : Item(name) {}

  Type type{Type::kDirectional};
  glm::vec3 color{1.0F, 1.0F, 1.0F};
  float intensity{1.0F};
  float range{100.0F};
  float innerSpotAngle{std::numbers::pi * 0.4F};
  float outerSpotAngle{std::numbers::pi * 0.5F};

  [[nodiscard]] auto getData(const glm::mat4& nodeGlobalTransform) const
      -> Data {
    const glm::vec3 position = glm::vec3(nodeGlobalTransform[3]);
    const glm::vec3 direction = glm::normalize(
        glm::vec3(nodeGlobalTransform * glm::vec4(0.0F, 0.0F, -1.0F, 0.0F)));

    return Data{
        .positionAndType = glm::vec4(position, static_cast<float>(type)),
        .colorAndIntensity = glm::vec4(color, intensity),
        .directionAndRange = glm::vec4(direction, range),
        .anglesAndPadding = glm::vec4(std::cos(innerSpotAngle),
                                      std::cos(outerSpotAngle), 0.0F, 0.0F)};
  }

  [[nodiscard]] static auto getViewMatrix(const glm::mat4& nodeGlobalTransform)
      -> glm::mat4 {
    const glm::vec3 position = glm::vec3(nodeGlobalTransform[3]);
    const glm::vec3 forward = glm::normalize(
        glm::vec3(nodeGlobalTransform * glm::vec4(0.0F, 0.0F, -1.0F, 0.0F)));
    const glm::vec3 up = glm::normalize(
        glm::vec3(nodeGlobalTransform * glm::vec4(0.0F, 1.0F, 0.0F, 0.0F)));

    return glm::lookAt(position, position + forward, up);
  }

  [[nodiscard]] auto getProjectionMatrix(float zNear = 0.1F) const
      -> glm::mat4 {
    glm::mat4 proj{1.0F};

    if (type == Type::kSpot) {
      proj = glm::perspective(outerSpotAngle * 2.0F, 1.0F, zNear, range);
    } else if (type == Type::kDirectional) {
      const float bounds = 20.0F;
      proj = glm::ortho(-bounds, bounds, -bounds, bounds, zNear, range);
    } else if (type == Type::kPoint) {
      proj = glm::perspective(static_cast<float>(std::numbers::pi) / 2.0F, 1.0F,
                              zNear, range);
    }

    proj[1][1] *= -1.0F;
    return proj;
  }

  [[nodiscard]] auto getViewProjectionMatrix(
      const glm::mat4& nodeGlobalTransform, float zNear = 0.1F) const
      -> glm::mat4 {
    return getProjectionMatrix(zNear) * getViewMatrix(nodeGlobalTransform);
  }
};

static_assert(sizeof(Light::Data) == 64,
              "Light::Data must be exactly 64 bytes for SSBO alignment");

};  // namespace vkit::scene
