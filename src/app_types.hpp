#pragma once

#include <cstdint>
#include <glm/glm.hpp>

namespace vkit {
struct Camera {
  glm::vec3 target{0.0F};
  float distance{5.0F};
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
  float iblIntensity{2.0F};

  std::int32_t diffuseEnvMapIdx{-1};
  std::int32_t specularEnvMapIdx{-1};
  float maxSpecularLod{9.0F};

  std::int32_t lightCount{0};
};

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
  float dissolveStrength;
};

struct ProceduralTextureParams {
  int patternType = 0;     // 0: Grid, 1: Bricks
  int generationMode = 0;  // 0: Color Only, 1: Normal Only, 2: Both

  int width = 512;
  int height = 512;
  glm::vec2 tileSize = {64.0F, 64.0F};
  float mortarThickness = 4.0F;

  glm::vec4 brickColor = {0.6F, 0.2F, 0.15F, 1.0F};
  glm::vec4 mortarColor = {0.2F, 0.2F, 0.2F, 1.0F};

  bool triggerGeneration = false;
};
};  // namespace vkit
