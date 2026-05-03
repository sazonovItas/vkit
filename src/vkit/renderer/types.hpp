#pragma once

namespace vkit::renderer::types {

struct alignas(16) CameraUBO {
  glm::mat4 view;
  glm::mat4 proj;
  alignas(16) glm::vec3 position;
};

};  // namespace vkit::renderer::types
