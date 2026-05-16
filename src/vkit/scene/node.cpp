#include "vkit/scene/node.hpp"

namespace vkit::scene {

Node::Node(std::string_view name) : Item(name) {}

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

auto Node::getParent() const -> std::weak_ptr<Node> { return parent_; }

auto Node::getChildren() const -> const std::vector<std::shared_ptr<Node>>& {
  return children_;
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

auto Node::getLocalTransform() const -> const TrsTransform& {
  return localTransform_;
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
