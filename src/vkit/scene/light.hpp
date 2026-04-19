#pragma once

namespace vkit::scene {

enum class LightType : std::uint32_t {
  kDirectional = 0,
  kPoint,
  kSpot,
};

class Light {
 public:
  using Type = LightType;

  static constexpr const char* kCTypeStrings[] = {
      "Directional",
      "Point",
      "Spot",
  };

  explicit Light(std::string_view name);

  Type type{Type::kDirectional};
  glm::vec3 color{1.0F, 1.0F, 1.0F};
  float intensity{1.0F};
  float range{100.0F};
  float innerSpotAngle{std::numbers::pi * 0.4F};
  float outerSpotAngle{std::numbers::pi * 0.5F};
};

};  // namespace vkit::scene
