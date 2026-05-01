#pragma once

#include <memory>

#include "vkit/scene/node.hpp"
#include "vkit/scene/node_storage.hpp"
#include "vkit/scene/scene.hpp"

namespace vkit::scene {

template <typename Callback, typename... Args>
void iterateSceneNodes(const Scene& scene, const NodeStorage& nodeStorage,
                       Callback&& callback, Args... initialArgs) {
  auto traverse = [&](auto& self, const std::shared_ptr<Node>& node,
                      Args... currentArgs) -> void {
    if (!node) return;

    callback(node, currentArgs...);

    for (const auto& child : node->getChildren()) {
      self(self, child, currentArgs...);
    }
  };

  for (std::uint32_t root_id : scene.rootNodes) {
    if (auto root_node = nodeStorage.get(root_id)) {
      traverse(traverse, root_node, initialArgs...);
    }
  }
}

};  // namespace vkit::scene
