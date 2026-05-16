#pragma once

#include <cstdint>
#include <optional>
#include <unordered_set>
#include <vector>

#include <glm/glm.hpp>

#include "vkit/renderer/types.hpp"

namespace vkit::controller {

enum class GizmoOp { None, Translate, Rotate, Scale };

class SceneController {
 public:
  SceneController();

  // --- Lights ---
  [[nodiscard]] auto getLights() const -> const std::vector<renderer::types::Light>&;
  [[nodiscard]] auto getLights() -> std::vector<renderer::types::Light>&;
  void addLight(const renderer::types::Light& light = {});
  void removeLight(int index);

  // --- Camera light ---
  [[nodiscard]] auto getCameraLight() -> renderer::types::Light&;
  [[nodiscard]] auto isCameraLightEnabled() const -> bool;
  void setCameraLightEnabled(bool enabled);
  void updateCameraPosition(glm::vec3 pos);

  // --- Scene params ---
  [[nodiscard]] auto getSceneParams() -> renderer::types::SceneParamsUBO&;

  // --- Skinning ---
  [[nodiscard]] auto isSkinningEnabled() const -> bool;
  void setSkinningEnabled(bool enabled);

  // --- Selection ---
  void selectPrimitive(std::uint32_t storageId);
  void deselectPrimitive(std::uint32_t storageId);
  void togglePrimitive(std::uint32_t storageId);
  void clearSelection();
  [[nodiscard]] auto isSelected(std::uint32_t storageId) const -> bool;
  [[nodiscard]] auto getSelection() const -> const std::unordered_set<std::uint32_t>&;

  // --- Material camera light (always at material camera position) ---
  [[nodiscard]] auto getMaterialCameraLight() -> renderer::types::Light&;
  [[nodiscard]] auto isMaterialCameraLightEnabled() const -> bool;
  void setMaterialCameraLightEnabled(bool enabled);
  void updateMaterialCameraPosition(glm::vec3 pos);

  // --- Gizmo mode ---
  [[nodiscard]] auto getGizmoOp() const -> GizmoOp;
  void setGizmoOp(GizmoOp op);

  // --- Selected light (for gizmo manipulation) ---
  [[nodiscard]] auto getSelectedLight() const -> std::optional<int>;
  void setSelectedLight(std::optional<int> index);

  // --- Build GPU data ---
  [[nodiscard]] auto buildSceneLightsSSBO() const -> renderer::types::LightsSSBO;
  [[nodiscard]] auto buildMatLightsSSBO() const -> renderer::types::LightsSSBO;

 private:
  std::vector<renderer::types::Light> lights_;
  renderer::types::SceneParamsUBO sceneParams_{};
  renderer::types::Light cameraLight_{};
  bool cameraLightEnabled_{false};
  bool skinningEnabled_{false};
  std::unordered_set<std::uint32_t> selectedPrimitives_{};

  renderer::types::Light materialCameraLight_{};
  bool materialCameraLightEnabled_{true};
  GizmoOp gizmoOp_{GizmoOp::Translate};
  std::optional<int> selectedLight_{std::nullopt};
};

}  // namespace vkit::controller
