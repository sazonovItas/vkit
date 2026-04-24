#pragma once

namespace vkit::scene {

class Node;

class NodeAttachment {
 public:
  void setNode(Node* node) { node_ = node; }
  [[nodiscard]] auto getNode() const -> Node* { return node_; }

 protected:
  Node* node_{nullptr};
};

};  // namespace vkit::scene
