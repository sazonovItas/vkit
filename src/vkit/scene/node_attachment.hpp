#pragma once

#include <string_view>

#include "vkit/scene/item.hpp"

namespace vkit::scene {

class Node;

enum class AttachmentType { kMesh, kCamera, kLight, kSkin, kUnknown };

class NodeAttachment : public Item {
 public:
  explicit NodeAttachment(std::string_view name) : Item(name) {}
  virtual ~NodeAttachment() noexcept = default;

  [[nodiscard]] virtual auto getType() const -> AttachmentType = 0;

  void setNode(Node* node) { node_ = node; }
  [[nodiscard]] auto getNode() const -> Node* { return node_; }

 protected:
  Node* node_{nullptr};
};

};  // namespace vkit::scene
