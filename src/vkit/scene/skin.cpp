#include "vkit/scene/skin.hpp"

#include "vkit/scene/node.hpp"

namespace vkit::scene {

void Skin::computeJointMatrices(std::span<glm::mat4> outMatrices) const {
  if (outMatrices.size() != joints.size()) {
    return;
  }

  for (std::size_t i = 0; i < joints.size(); ++i) {
    if (joints[i]) {
      const auto& joint_global = joints[i]->getGlobalTransform().getMatrix();

      outMatrices[i] = joint_global * inverseBindMatrices[i];
    } else {
      outMatrices[i] = glm::mat4{1.0F};
    }
  }
}

};  // namespace vkit::scene
