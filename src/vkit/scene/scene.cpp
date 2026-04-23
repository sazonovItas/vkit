#include "vkit/scene/scene.hpp"

namespace vkit::scene {

Scene::Scene(std::string_view name) : Item(name) {}

void Scene::addRootNode(const std::shared_ptr<Node>& node) {
  if (node) {
    rootNodes_.push_back(node);
  }
}

void Scene::removeRootNode(const std::shared_ptr<Node>& node) {
  std::erase(rootNodes_, node);
}

auto Scene::getRootNodes() const -> const std::vector<std::shared_ptr<Node>>& {
  return rootNodes_;
}

void Scene::registerNode(const std::shared_ptr<Node>& node) {
  if (node) nodes_.push_back(node);
}

void Scene::unregisterNode(const std::shared_ptr<Node>& node) {
  std::erase(nodes_, node);
}

void Scene::registerMesh(const std::shared_ptr<Mesh>& mesh) {
  if (mesh) meshes_.push_back(mesh);
}

void Scene::unregisterMesh(const std::shared_ptr<Mesh>& mesh) {
  std::erase(meshes_, mesh);
}

void Scene::registerLight(const std::shared_ptr<Light>& light) {
  if (light) lights_.push_back(light);
}

void Scene::unregisterLight(const std::shared_ptr<Light>& light) {
  std::erase(lights_, light);
}

void Scene::registerCamera(const std::shared_ptr<Camera>& camera) {
  if (camera) cameras_.push_back(camera);
}

void Scene::unregisterCamera(const std::shared_ptr<Camera>& camera) {
  std::erase(cameras_, camera);
}

auto Scene::getNodes() const -> const std::vector<std::shared_ptr<Node>>& {
  return nodes_;
}
auto Scene::getMeshes() const -> const std::vector<std::shared_ptr<Mesh>>& {
  return meshes_;
}
auto Scene::getLights() const -> const std::vector<std::shared_ptr<Light>>& {
  return lights_;
}
auto Scene::getCameras() const -> const std::vector<std::shared_ptr<Camera>>& {
  return cameras_;
}

};  // namespace vkit::scene
