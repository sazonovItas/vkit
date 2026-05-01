#pragma once

#include <cstdint>
#include <string_view>
#include <vector>

#include "vkit/item/item.hpp"

namespace vkit::scene {

class Scene : public Item<Scene> {
 public:
  explicit Scene(std::string_view name = "Scene") : Item(name) {}

  std::vector<std::uint32_t> rootNodes;

  void addRootNode(std::uint32_t nodeId) { rootNodes.push_back(nodeId); }
};

};  // namespace vkit::scene
