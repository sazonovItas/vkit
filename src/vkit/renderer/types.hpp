#pragma once

#include <array>
#include <cstdint>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace vkit::renderer::types {

struct alignas(16) CameraUBO {
  glm::mat4 view;
  glm::mat4 proj;
  alignas(16) glm::vec3 position;
};

// shadowViewProj is the light-space matrix for the primary shadow-casting light.
// Storing it here avoids a separate UBO binding in fragment shaders.
struct alignas(16) SceneParamsUBO {
  float exposure{1.5F};
  float gamma{1.0F};
  int shadowsEnabled{0};
  float shadowBias{0.005F};
  glm::mat4 shadowViewProj{1.0F};
};

// Used only by the shadow-map depth pipeline (shadow.vert needs just viewProj).
struct alignas(16) ShadowLightUBO {
  glm::mat4 viewProj{1.0F};
};

enum class LightType : std::int32_t {
  kDirectional = 0,
  kPoint       = 1,
  kSpot        = 2,
};

struct alignas(16) Light {
  alignas(16) glm::vec3 position{0.0F, 5.0F, 0.0F};
  float range{50.0F};
  alignas(16) glm::vec3 direction{glm::normalize(glm::vec3(-1.0F, -2.0F, -1.0F))};
  float intensity{1.0F};
  alignas(16) glm::vec3 color{1.0F, 1.0F, 1.0F};
  std::int32_t type{static_cast<std::int32_t>(LightType::kDirectional)};
  float innerAngle{0.35F};   // radians, for spot
  float outerAngle{0.65F};   // radians, for spot
  std::int32_t castsShadows{1};
  float _pad{0.0F};
};

static constexpr std::uint32_t kMaxSceneLights = 16;

struct LightsSSBO {
  std::uint32_t count{0};
  std::uint32_t _pad0{0};
  std::uint32_t _pad1{0};
  std::uint32_t _pad2{0};
  std::array<Light, kMaxSceneLights> lights{};
};

};  // namespace vkit::renderer::types
