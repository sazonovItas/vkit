#pragma once

#include <memory>
#include <vector>

#include "vkit/item/item.hpp"
#include "vkit/item/storage_item.hpp"
#include "vkit/scene/trs_transform.hpp"

namespace vkit::scene {

class Mesh;
class Light;
class Camera;
class Skin;

};  // namespace vkit::scene

namespace vkit::scene {

class Node : public Item<Node>,
             public StorageItem,
             public std::enable_shared_from_this<Node> {
 public:
  explicit Node(std::string_view name = "Node");

  std::shared_ptr<Mesh> mesh;
  std::shared_ptr<Light> light;
  std::shared_ptr<Camera> camera;
  std::shared_ptr<Skin> skin;

  [[nodiscard]] auto getParent() const -> std::weak_ptr<Node>;
  [[nodiscard]] auto getChildren() const
      -> const std::vector<std::shared_ptr<Node>>&;

  void setParent(const std::shared_ptr<Node>& newParent);
  void addChild(const std::shared_ptr<Node>& child);

  void setLocalTransform(const TrsTransform& transform);
  void invalidateTransform() const;

  [[nodiscard]] auto getLocalTransform() const -> const TrsTransform&;
  [[nodiscard]] auto getGlobalTransform() const -> const TrsTransform&;

 private:
  TrsTransform localTransform_;
  mutable TrsTransform worldTransform_;
  mutable bool transformDirty_{true};

  std::weak_ptr<Node> parent_;
  std::vector<std::shared_ptr<Node>> children_;
};

};  // namespace vkit::scene
