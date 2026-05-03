#include "vkit/workflow/workflow.hpp"

#include <algorithm>
#include <queue>
#include <unordered_map>

namespace vkit::workflow {

void Workflow::execute() {
  if (!isDirty_) return;
  isDirty_ = false;

  std::unordered_map<graph::Node*, int> in_degree;
  std::unordered_map<graph::Node*, bool> is_complete;

  for (auto* node : nodes) {
    in_degree.emplace(node, 0);
    is_complete.emplace(node, false);
  }

  for (auto& link : links) {
    if (auto* sink_node = link->getSink()->getOwnerNode()) {
      in_degree[sink_node]++;
    }
  }

  std::queue<graph::Node*> queue;
  for (auto& [node, deg] : in_degree) {
    if (deg == 0) queue.push(node);
  }

  while (!queue.empty()) {
    auto* node = queue.front();
    queue.pop();

    auto* wn = dynamic_cast<WorkflowNode*>(node);

    if (!wn || wn->status() == NodeStatus::kReady) {
      is_complete[node] = true;
    } else if (wn->status() == NodeStatus::kExecuting) {
      is_complete[node] = false;
    } else {
      bool all_ready = true;
      for (auto& pin : node->getInputs()) {
        for (auto* link : pin->getLinks()) {
          if (!is_complete[link->getSrc()->getOwnerNode()]) {
            all_ready = false;
            break;
          }
        }
        if (!all_ready) break;
      }

      if (all_ready) {
        wn->execute();
      }
      is_complete[node] = false;
    }

    if (!is_complete[node]) continue;

    for (auto& pin : node->getOutputs()) {
      for (auto* link : pin->getLinks()) {
        auto* downstream = link->getSink()->getOwnerNode();
        if (downstream && --in_degree[downstream] == 0) {
          queue.push(downstream);
        }
      }
    }
  }
}

auto Workflow::findPin(int id) -> graph::Pin* {
  for (auto* node : nodes) {
    for (auto& pin : node->getInputs()) {
      if (pin->getId() == id) return pin.get();
    }
    for (auto& pin : node->getOutputs()) {
      if (pin->getId() == id) return pin.get();
    }
  }
  return nullptr;
}

auto Workflow::findLink(int id) -> graph::Link* {
  for (auto& link : links) {
    if (link->getId() == id) return link.get();
  }
  return nullptr;
}

auto Workflow::canConnect(graph::Pin* a, graph::Pin* b) const -> bool {
  if (!a || !b) return false;

  if (a == b) return false;
  if (a->getOwnerNode() == b->getOwnerNode()) return false;
  if (a->getKind() == b->getKind()) return false;

  auto* input_pin = (a->getKind() == graph::PinKind::kInput) ? a : b;
  auto* output_pin = (a->getKind() == graph::PinKind::kOutput) ? a : b;

  if (!input_pin->getLinks().empty()) return false;
  if (input_pin->getKey() != output_pin->getKey()) return false;

  auto* src_node = output_pin->getOwnerNode();
  auto* dst_node = input_pin->getOwnerNode();

  return !wouldCreateCycle(src_node, dst_node);
}

auto Workflow::connect(graph::Pin* src, graph::Pin* sink) -> graph::Link* {
  auto* link = graph::Graph::connect(src, sink);

  if (link && sink) {
    if (auto* target_node = static_cast<WorkflowNode*>(sink->getOwnerNode())) {
      target_node->markStale();
    }
  }

  markDirty();
  return link;
}

void Workflow::disconnect(graph::Link* link) {
  if (!link) return;

  auto* dst_pin = link->getSink();
  WorkflowNode* target_node = nullptr;
  if (dst_pin) {
    target_node = static_cast<WorkflowNode*>(dst_pin->getOwnerNode());
  }

  graph::Graph::disconnect(link);

  if (target_node) {
    target_node->markStale();
  }

  markDirty();
}

void Workflow::destroyNode(int nodeId) {
  auto it = std::ranges::find_if(
      nodes, [nodeId](const graph::Node* n) { return n->getId() == nodeId; });
  if (it == nodes.end()) return;

  auto* node = *it;

  auto disconnect_all = [this](auto& pins) {
    for (auto& pin : pins) {
      auto links_copy = pin->getLinks();
      for (auto* link : links_copy) {
        disconnect(link);
      }
    }
  };
  disconnect_all(node->getInputs());
  disconnect_all(node->getOutputs());

  unregisterNode(node);

  auto owned_it = std::ranges::find_if(
      ownedNodes_, [nodeId](const auto& n) { return n->getId() == nodeId; });
  if (owned_it != ownedNodes_.end()) {
    ownedNodes_.erase(owned_it);
  }

  markDirty();
}

auto Workflow::wouldCreateCycle(graph::Node* sourceNode,
                                graph::Node* targetNode) const -> bool {
  if (sourceNode == targetNode) return true;

  std::unordered_set<graph::Node*> visited;
  std::queue<graph::Node*> queue;

  queue.push(targetNode);
  visited.insert(targetNode);

  while (!queue.empty()) {
    auto* current = queue.front();
    queue.pop();

    if (current == sourceNode) {
      return true;
    }

    for (auto& pin : current->getOutputs()) {
      for (auto* link : pin->getLinks()) {
        auto* downstream_node = link->getSink()->getOwnerNode();
        if ((downstream_node != nullptr) &&
            visited.insert(downstream_node).second) {
          queue.push(downstream_node);
        }
      }
    }
  }

  return false;
}

};  // namespace vkit::workflow
