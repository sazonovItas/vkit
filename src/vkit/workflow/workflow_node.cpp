#include "vkit/workflow/workflow_node.hpp"

#include "vkit/graph/link.hpp"
#include "vkit/graph/pin.hpp"

namespace vkit::workflow {

void WorkflowNode::setStatus(NodeStatus s) {
  if (status_ == s) return;
  status_ = s;

  if (onStateChanged) {
    onStateChanged();
  }
}

void WorkflowNode::markStale() {
  if (status_ == NodeStatus::kStale) {
    if (onStateChanged) {
      onStateChanged();
    }
    return;
  }

  setStatus(NodeStatus::kStale);
  propagateStale();
}

void WorkflowNode::propagateStale() {
  std::unordered_set<WorkflowNode*> visited;
  propagateStaleRecursive(visited);
}

void WorkflowNode::propagateStaleRecursive(
    std::unordered_set<WorkflowNode*>& visited) {
  if (!visited.insert(this).second) return;

  for (auto& pin : outputPins_) {
    for (auto* link : pin->getLinks()) {
      auto* downstream = link->getSink()->getOwnerNode();
      if (auto* wn = dynamic_cast<WorkflowNode*>(downstream)) {
        wn->setStatus(NodeStatus::kStale);
        wn->propagateStaleRecursive(visited);
      }
    }
  }
}

};  // namespace vkit::workflow
