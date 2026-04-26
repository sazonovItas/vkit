#pragma once

namespace vkit::primitive {

class Primitive;

class Attachment {
 public:
  void setNode(Primitive* node) { node_ = node; }
  [[nodiscard]] auto getNode() const -> Primitive* { return node_; }

 protected:
  Primitive* node_{nullptr};
};

};  // namespace vkit::primitive
