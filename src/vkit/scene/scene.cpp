#include "vkit/scene/scene.hpp"

namespace vkit::scene {

Scene::Scene(std::string_view name) : Item(name) {}

auto Scene::getRootNodes() const -> std::span<const std::shared_ptr<Node>> {
  return rootNodes_;
}

auto Scene::getNodes() const -> std::span<const std::shared_ptr<Node>> {
  return nodes_;
}

auto Scene::getMeshes() const -> std::span<const std::shared_ptr<Mesh>> {
  return meshes_;
}

auto Scene::getLights() const -> std::span<const std::shared_ptr<Light>> {
  return lights_;
}

auto Scene::getCameras() const -> std::span<const std::shared_ptr<Camera>> {
  return cameras_;
}

void Scene::addRootNode(const std::shared_ptr<Node>& node) {
  if (node) {
    rootNodes_.push_back(node);
  }
}

void Scene::removeRootNode(const std::shared_ptr<Node>& node) {
  std::erase(rootNodes_, node);
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

void Scene::registerSkin(const std::shared_ptr<Skin>& skin) {
  if (skin) skinsStorage_.add(skin);
}

void Scene::unregisterSkin(const std::shared_ptr<Skin>& skin) {
  skinsStorage_.remove(skin);
}

};  // namespace vkit::scene
