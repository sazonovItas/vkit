#pragma once

#include <memory>
#include <utility>
#include <vector>

#include "vkit/graph/graph.hpp"
#include "vkit/graph/link.hpp"
#include "vkit/graph/node.hpp"
#include "vkit/graph/pin.hpp"
#include "vkit/workflow/workflow_node.hpp"

namespace vkit::workflow {

class Workflow : public graph::Graph {
 public:
  void execute();

  void markDirty() { isDirty_ = true; }
  [[nodiscard]] auto isDirty() const -> bool { return isDirty_; }

  [[nodiscard]] auto findPin(int id) -> graph::Pin*;
  [[nodiscard]] auto findLink(int id) -> graph::Link*;

  [[nodiscard]] auto canConnect(graph::Pin* a, graph::Pin* b) const -> bool;

  template <typename NodeT, typename... Args>
  auto createNode(Args&&... args) -> NodeT* {
    auto node = std::make_unique<NodeT>(std::forward<Args>(args)...);
    NodeT* ptr = node.get();

    ptr->onStateChanged = [this]() { this->markDirty(); };

    registerNode(ptr);
    ownedNodes_.push_back(std::move(node));

    markDirty();

    return ptr;
  }

  void destroyNode(int nodeId);

 private:
  bool isDirty_{true};
  std::vector<std::unique_ptr<WorkflowNode>> ownedNodes_;

  [[nodiscard]] auto wouldCreateCycle(graph::Node* sourceNode,
                                      graph::Node* targetNode) const -> bool;
};

};  // namespace vkit::workflow
