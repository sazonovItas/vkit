#pragma once

#include <glm/glm.hpp>
#include <memory>
#include <span>
#include <string_view>
#include <vector>

#include "vkit/item/item.hpp"
#include "vkit/item/storage_item.hpp"

namespace vkit::scene {

class Node;

class Skin : public Item<Skin>, public StorageItem {
 public:
  explicit Skin(std::string_view name = "Skin") : Item{name} {}

  std::shared_ptr<Node> skeleton;
  std::vector<std::shared_ptr<Node>> joints;
  std::vector<glm::mat4> inverseBindMatrices;

  void computeJointMatrices(std::span<glm::mat4> outMatrices) const;
};

};  // namespace vkit::scene
