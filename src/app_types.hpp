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
  glm::vec3 lightDir{0.0, -1.0, -1.0};
  float exposure{1.0};
  float gamma{2.2};
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
