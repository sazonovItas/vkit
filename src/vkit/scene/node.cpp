#include "vkit/scene/node.hpp"

namespace vkit::scene {

Node::Node(std::string_view name) : Item(name) {}

void Node::attach(const std::shared_ptr<NodeAttachment>& attachment) {
  if (attachment) {
    attachment->setNode(this);
    attachments_.push_back(attachment);
  }
}

void Node::detach(NodeAttachment* attachment) {
  std::erase_if(attachments_,
                [attachment](const std::shared_ptr<NodeAttachment>& a) {
                  if (a.get() == attachment) {
                    a->setNode(nullptr);
                    return true;
                  }
                  return false;
                });
}

void Node::setParent(const std::shared_ptr<Node>& newParent) {
  parent_ = newParent;
  invalidateTransform();
}

void Node::addChild(const std::shared_ptr<Node>& child) {
  if (child) {
    child->setParent(shared_from_this());
    children_.push_back(child);
  }
}

void Node::setLocalTransform(const TrsTransform& transform) {
  localTransform_ = transform;
  invalidateTransform();
}

void Node::invalidateTransform() const {
  if (transformDirty_) return;

  transformDirty_ = true;
  for (const auto& child : children_) {
    child->invalidateTransform();
  }
}

auto Node::getGlobalTransform() const -> const TrsTransform& {
  if (transformDirty_) {
    if (auto p = parent_.lock()) {
      worldTransform_ = p->getGlobalTransform() * localTransform_;
    } else {
      worldTransform_ = localTransform_;
    }
    transformDirty_ = false;
  }
  return worldTransform_;
}

};  // namespace vkit::scene
