#pragma once

#include <memory>
#include <string_view>
#include <vector>

#include "vkit/scene/camera.hpp"
#include "vkit/scene/item.hpp"
#include "vkit/scene/light.hpp"
#include "vkit/scene/mesh.hpp"
#include "vkit/scene/node.hpp"
#include "vkit/scene/skin.hpp"

namespace vkit::scene {

class Scene : public Item {
 public:
  explicit Scene(std::string_view name = "Scene");

  void addRootNode(const std::shared_ptr<Node>& node);
  void removeRootNode(const std::shared_ptr<Node>& node);

  [[nodiscard]] auto getRootNodes() const
      -> const std::vector<std::shared_ptr<Node>>&;
  [[nodiscard]] auto getNodes() const
      -> const std::vector<std::shared_ptr<Node>>&;
  [[nodiscard]] auto getMeshes() const
      -> const std::vector<std::shared_ptr<Mesh>>&;
  [[nodiscard]] auto getLights() const
      -> const std::vector<std::shared_ptr<Light>>&;
  [[nodiscard]] auto getCameras() const
      -> const std::vector<std::shared_ptr<Camera>>&;
  [[nodiscard]] auto getSkins() const
      -> const std::vector<std::shared_ptr<Skin>>&;

  void registerNode(const std::shared_ptr<Node>& node);
  void unregisterNode(const std::shared_ptr<Node>& node);
  void registerMesh(const std::shared_ptr<Mesh>& mesh);
  void unregisterMesh(const std::shared_ptr<Mesh>& mesh);
  void registerLight(const std::shared_ptr<Light>& light);
  void unregisterLight(const std::shared_ptr<Light>& light);
  void registerCamera(const std::shared_ptr<Camera>& camera);
  void unregisterCamera(const std::shared_ptr<Camera>& camera);
  void registerSkin(const std::shared_ptr<Skin>& skin);
  void unregisterSkin(const std::shared_ptr<Skin>& skin);

 private:
  std::vector<std::shared_ptr<Node>> rootNodes_;

  std::vector<std::shared_ptr<Node>> nodes_;
  std::vector<std::shared_ptr<Mesh>> meshes_;
  std::vector<std::shared_ptr<Skin>> skins_;
  std::vector<std::shared_ptr<Light>> lights_;
  std::vector<std::shared_ptr<Camera>> cameras_;
};

};  // namespace vkit::scene
