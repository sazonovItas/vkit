#pragma once

#include <glm/glm.hpp>
#include <memory>
#include <string_view>
#include <vector>

#include "vkit/item/item.hpp"
#include "vkit/scene/node.hpp"
#include "vkit/scene/node_attachment.hpp"
#include "vkit/scene/trs_transform.hpp"

namespace vkit::scene {

class Node;

class Skin : public Item<Skin>, public NodeAttachment {
 public:
  explicit Skin(std::string_view name = "Skin") : Item{name} {}

  std::shared_ptr<Node> skeleton;
  std::vector<std::shared_ptr<Node>> joints;
  std::vector<glm::mat4> inverseBindMatrices;
  std::vector<glm::mat4> jointMatrices;

  void update(const TrsTransform& meshTransform) {
    if (jointMatrices.size() != joints.size()) {
      jointMatrices.resize(joints.size());
    }

    for (std::size_t i = 0; i < joints.size(); ++i) {
      auto joint_global = joints[i]->getGlobalTransform();

      jointMatrices[i] = meshTransform.getInverseMatrix() *
                         joint_global.getMatrix() * inverseBindMatrices[i];
    }
  }
};

};  // namespace vkit::scene
