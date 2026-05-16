#pragma once

namespace vkit::renderer::types {

struct alignas(16) CameraUBO {
  glm::mat4 view;
  glm::mat4 proj;
  alignas(16) glm::vec3 position;
};

struct alignas(16) SceneParamsUBO {
  float exposure{1.5F};
  float gamma{1.0F};
  float _pad0{0.0F};
  float _pad1{0.0F};
};

};  // namespace vkit::renderer::types
