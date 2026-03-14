#pragma once

namespace vkit {
struct Camera {
  glm::vec3 target{0.0F};
  float distance{2.0F};
  float yaw{45.0F};
  float pitch{30.0F};
  glm::vec3 up{0.0F, 1.0F, 0.0F};

  glm::vec3 getPosition() const {
    float x = distance * cos(glm::radians(pitch)) * cos(glm::radians(yaw));
    float y = distance * sin(glm::radians(pitch));
    float z = distance * cos(glm::radians(pitch)) * sin(glm::radians(yaw));
    return target + glm::vec3(x, y, z);
  }
};

struct UBO {
  glm::mat4 model;
  glm::mat4 view;
  glm::mat4 projection;
  alignas(16) glm::vec3 cameraPosition;
};

struct UBOParams {
  float exposure{1.0F};
  float gamma{2.2F};
  std::int32_t diffuseEnvMapIdx{-1};
  std::int32_t specularEnvMapIdx{-1};

  std::int32_t brdfLUTIdx{-1};
  float maxSpecularLod{9.0F};
  std::int32_t lightCount{0};
};

#include <cstdint>
#include <glm/glm.hpp>

enum class LightType : std::uint32_t {
  kDirectional = 0,
  kPoint = 1,
  kSpot = 2
};

struct Light {
  glm::vec3 position{0.0F};
  std::uint32_t type{0};

  glm::vec3 direction{0.0F, 0.0F, -1.0F};
  float range{0.0F};

  glm::vec3 color{1.0F};
  float intensity{1.0F};

  glm::vec2 scaleOffset{1.0F, 0.0F};
  std::uint32_t shadowMapID{0};
  float unused{0.0F};
};

struct alignas(16) Material {
  glm::vec4 baseColorFactor;
  glm::vec4 emissiveFactor;

  float metallicFactor;
  float roughnessFactor;
  float alphaMaskCutoff;
  float emissiveStrength;

  int32_t baseColorTextureIdx;
  int32_t metallicRoughnessTextureIdx;
  int32_t normalTextureIdx;
  int32_t occlusionTextureIdx;

  int32_t emissiveTextureIdx;
  int32_t pad0;
  int32_t pad1;
  int32_t pad2;
};
};  // namespace vkit
