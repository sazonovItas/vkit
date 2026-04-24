#pragma once

#include <glm/glm.hpp>
#include <memory>
#include <vector>

#include "vkit/item/item.hpp"
#include "vkit/scene/node_attachment.hpp"

namespace vkit::scene {

class Node;

class Skin : public Item<Skin>, public NodeAttachment {
 public:
  explicit Skin(std::string_view name = "Skin") : Item{name} {}

  std::weak_ptr<Node> skeletonRoot;
  std::vector<std::weak_ptr<Node>> joints;
  std::vector<glm::mat4> inverseBindMatrices;
};

};  // namespace vkit::scene
