#pragma once

#include <memory>

#include "vkit/scene/node.hpp"
#include "vkit/scene/scene.hpp"

namespace vkit::scene {

template <typename Callback, typename... Args>
void iterateSceneNodes(const Scene& scene, Callback&& callback,
                       Args... initialArgs) {
  auto traverse = [&](auto& self, const std::shared_ptr<Node>& node,
                      Args... currentArgs) -> void {
    if (!node) return;

    callback(node, currentArgs...);

    for (const auto& child : node->getChildren()) {
      self(self, child, currentArgs...);
    }
  };

  for (const auto& root_node : scene.getRootNodes()) {
    traverse(traverse, root_node, initialArgs...);
  }
}

};  // namespace vkit::scene
