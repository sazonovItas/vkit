#pragma once

#include <memory>
#include <string_view>
#include <vector>

#include "vkit/item/item.hpp"
#include "vkit/item/storage_item.hpp"
#include "vkit/primitive/storage.hpp"
#include "vkit/scene/camera_storage.hpp"
#include "vkit/scene/light_storage.hpp"
#include "vkit/scene/mesh_storage.hpp"
#include "vkit/scene/node_storage.hpp"
#include "vkit/scene/scene.hpp"
#include "vkit/scene/skin_storage.hpp"

namespace vkit::asset {

class Asset : public Item<Asset>, public StorageItem {
 public:
  explicit Asset(std::string_view name = "Asset") : Item(name) {}

  scene::NodeStorage nodes;
  scene::MeshStorage meshes;
  scene::CameraStorage cameras;
  scene::LightStorage lights;
  scene::SkinStorage skins;

  primitive::PrimitiveStorage primitives;

  std::vector<std::shared_ptr<scene::Scene>> scenes;
  std::int32_t activeSceneIndex{0};

  [[nodiscard]] auto getActiveScene() const -> std::shared_ptr<scene::Scene> {
    return scenes[activeSceneIndex];
  }
  void setActiveScene(const std::uint32_t idx) { activeSceneIndex = idx; }

  void update() {
    primitives.update();

    auto active_nodes = nodes.getItems();
    lights.update(active_nodes);
    skins.update(active_nodes);
  }
};

};  // namespace vkit::asset
