#pragma once

#include <memory>
#include <vector>

#include "vkit/scene/item.hpp"
#include "vkit/scene/node_attachment.hpp"
#include "vkit/scene/trs_transform.hpp"

namespace vkit::scene {

class Node : public Item, public std::enable_shared_from_this<Node> {
 public:
  explicit Node(std::string_view name = "Node");

  void attach(const std::shared_ptr<NodeAttachment>& attachment);
  void detach(NodeAttachment* attachment);

  [[nodiscard]] auto getAttachments() const
      -> const std::vector<std::shared_ptr<NodeAttachment>>& {
    return attachments_;
  }

  template <typename T>
  [[nodiscard]] auto getAttachment() const -> std::shared_ptr<T> {
    for (const auto& attachment : attachments_) {
      if (auto result = std::dynamic_pointer_cast<T>(attachment)) {
        return result;
      }
    }
    return nullptr;
  }

  [[nodiscard]] auto getParent() const -> std::weak_ptr<Node> {
    return parent_;
  }
  [[nodiscard]] auto getChildren() const
      -> const std::vector<std::shared_ptr<Node>>& {
    return children_;
  }

  void setParent(const std::shared_ptr<Node>& newParent);
  void addChild(const std::shared_ptr<Node>& child);

  void setLocalTransform(const TrsTransform& transform);
  void invalidateTransform() const;

  [[nodiscard]] auto getGlobalTransform() const -> const TrsTransform&;

 private:
  TrsTransform localTransform_;
  mutable TrsTransform worldTransform_;
  mutable bool transformDirty_{true};

  std::weak_ptr<Node> parent_;
  std::vector<std::shared_ptr<Node>> children_;
  std::vector<std::shared_ptr<NodeAttachment>> attachments_;
};

};  // namespace vkit::scene
