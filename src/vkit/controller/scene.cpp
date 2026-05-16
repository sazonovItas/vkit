#include "vkit/controller/scene.hpp"

#include <algorithm>

namespace vkit::controller {

SceneController::SceneController() {
  lights_.push_back({});

  cameraLight_.type =
      static_cast<int>(renderer::types::LightType::kPoint);
  cameraLight_.color = {1.0F, 1.0F, 1.0F};
  cameraLight_.intensity = 1.5F;
  cameraLight_.range = 25.0F;
  cameraLight_.castsShadows = 0;

  // Material camera light — follows the material camera each frame.
  materialCameraLight_.type         = static_cast<int>(renderer::types::LightType::kPoint);
  materialCameraLight_.color        = {1.0F, 1.0F, 1.0F};
  materialCameraLight_.intensity    = 1.5F;
  materialCameraLight_.range        = 100.0F;
  materialCameraLight_.castsShadows = 0;
}

auto SceneController::getLights() const
    -> const std::vector<renderer::types::Light>& {
  return lights_;
}

auto SceneController::getLights() -> std::vector<renderer::types::Light>& {
  return lights_;
}

void SceneController::addLight(const renderer::types::Light& light) {
  if (lights_.size() < renderer::types::kMaxSceneLights)
    lights_.push_back(light);
}

void SceneController::removeLight(int index) {
  if (index >= 0 && static_cast<std::size_t>(index) < lights_.size())
    lights_.erase(lights_.begin() + index);
}

auto SceneController::getCameraLight() -> renderer::types::Light& {
  return cameraLight_;
}

auto SceneController::isCameraLightEnabled() const -> bool {
  return cameraLightEnabled_;
}

void SceneController::setCameraLightEnabled(bool enabled) {
  cameraLightEnabled_ = enabled;
}

void SceneController::updateCameraPosition(glm::vec3 pos) {
  cameraLight_.position = pos;
}

auto SceneController::getSceneParams() -> renderer::types::SceneParamsUBO& {
  return sceneParams_;
}

auto SceneController::isSkinningEnabled() const -> bool {
  return skinningEnabled_;
}

void SceneController::setSkinningEnabled(bool enabled) {
  skinningEnabled_ = enabled;
}

void SceneController::selectPrimitive(std::uint32_t storageId) {
  selectedPrimitives_.insert(storageId);
}

void SceneController::deselectPrimitive(std::uint32_t storageId) {
  selectedPrimitives_.erase(storageId);
}

void SceneController::togglePrimitive(std::uint32_t storageId) {
  if (selectedPrimitives_.count(storageId))
    selectedPrimitives_.erase(storageId);
  else
    selectedPrimitives_.insert(storageId);
}

void SceneController::clearSelection() {
  selectedPrimitives_.clear();
}

auto SceneController::isSelected(std::uint32_t storageId) const -> bool {
  return selectedPrimitives_.count(storageId) > 0;
}

auto SceneController::getSelection() const
    -> const std::unordered_set<std::uint32_t>& {
  return selectedPrimitives_;
}

auto SceneController::getMaterialCameraLight() -> renderer::types::Light& {
  return materialCameraLight_;
}

auto SceneController::isMaterialCameraLightEnabled() const -> bool {
  return materialCameraLightEnabled_;
}

void SceneController::setMaterialCameraLightEnabled(bool enabled) {
  materialCameraLightEnabled_ = enabled;
}

void SceneController::updateMaterialCameraPosition(glm::vec3 pos) {
  materialCameraLight_.position = pos;
}

auto SceneController::getGizmoOp() const -> GizmoOp { return gizmoOp_; }
void SceneController::setGizmoOp(GizmoOp op) { gizmoOp_ = op; }

auto SceneController::getSelectedLight() const -> std::optional<int> {
  return selectedLight_;
}
void SceneController::setSelectedLight(std::optional<int> index) {
  selectedLight_ = index;
}

auto SceneController::buildSceneLightsSSBO() const
    -> renderer::types::LightsSSBO {
  renderer::types::LightsSSBO ssbo{};
  std::uint32_t count = 0;
  for (const auto& l : lights_) {
    if (count >= renderer::types::kMaxSceneLights) break;
    ssbo.lights[count++] = l;
  }
  if (cameraLightEnabled_ && count < renderer::types::kMaxSceneLights)
    ssbo.lights[count++] = cameraLight_;
  ssbo.count = count;
  return ssbo;
}

auto SceneController::buildMatLightsSSBO() const
    -> renderer::types::LightsSSBO {
  renderer::types::LightsSSBO ssbo{};
  if (materialCameraLightEnabled_) {
    ssbo.lights[0] = materialCameraLight_;
    ssbo.count = 1;
  }
  return ssbo;
}

}  // namespace vkit::controller
