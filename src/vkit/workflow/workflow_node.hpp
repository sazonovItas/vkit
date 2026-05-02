#pragma once
#include <unordered_set>

#include "vkit/graph/node.hpp"
#include "vkit/workflow/node_status.hpp"

namespace vkit::workflow {

class WorkflowNode : public graph::Node {
 public:
  explicit WorkflowNode(const std::string_view name) : name_{name} {};
  ~WorkflowNode() override = default;

  [[nodiscard]] auto getName() const -> const std::string& { return name_; }
  void setName(const std::string& name) { name_ = name; }

  virtual void execute() {};
  [[nodiscard]] auto status() const -> NodeStatus { return status_; }

  void markStale();

  std::function<void()> onStateChanged;

 protected:
  std::string name_;

  NodeStatus status_{NodeStatus::kStale};
  void setStatus(NodeStatus s);
  void propagateStale();

 private:
  void propagateStaleRecursive(std::unordered_set<WorkflowNode*>& visited);
};

};  // namespace vkit::workflow
