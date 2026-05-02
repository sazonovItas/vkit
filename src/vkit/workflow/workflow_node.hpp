#pragma once
#include <unordered_set>

#include "vkit/graph/node.hpp"
#include "vkit/workflow/node_status.hpp"

namespace vkit::workflow {

class WorkflowNode : public graph::Node {
 public:
  using ::vkit::graph::Node::Node;
  ~WorkflowNode() override = default;

  virtual void execute() {};
  [[nodiscard]] auto status() const -> NodeStatus { return status_; }
  void markStale();

  std::function<void()> onStateChanged;

 protected:
  NodeStatus status_{NodeStatus::kStale};
  void setStatus(NodeStatus s);
  void propagateStale();

 private:
  void propagateStaleRecursive(std::unordered_set<WorkflowNode*>& visited);
};

};  // namespace vkit::workflow
