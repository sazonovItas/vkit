#include "vkit/imgui/windows/configuration.hpp"

#include <imgui.h>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>
#include <string>
#include <unordered_map>

#include <glm/gtc/quaternion.hpp>

#include "vkit/animation/animator.hpp"
#include "vkit/controller/asset.hpp"
#include "vkit/controller/environment.hpp"
#include "vkit/controller/workflow.hpp"
#include "vkit/environment/environment.hpp"
#include "vkit/material/manager.hpp"
#include "vkit/scene/node.hpp"
#include "vkit/scene/trs_transform.hpp"

namespace vkit::imgui::windows {

ConfigurationWindow::ConfigurationWindow(std::string_view title)
    : ImguiWindow(title) {
  setMinSize(300.0F, 400.0F);
}

void ConfigurationWindow::setAssetController(
    controller::AssetController* assetController) {
  assetController_ = assetController;
}

void ConfigurationWindow::setEnvironmentController(
    controller::EnvironmentController* envController) {
  envController_ = envController;
}

void ConfigurationWindow::setSceneController(
    controller::SceneController* sceneController) {
  sceneController_ = sceneController;
}

void ConfigurationWindow::setAnimator(animation::Animator* animator,
                                      bool* enableSkinning) {
  animator_ = animator;
  enableSkinning_ = enableSkinning;
}

void ConfigurationWindow::setMaterialPreviewData(
    material::MaterialManager* matManager, std::uint32_t* previewSlot) {
  matManager_ = matManager;
  previewSlot_ = previewSlot;
}

void ConfigurationWindow::setWorkflowController(
    controller::WorkflowController* workflowController) {
  workflowController_ = workflowController;
}

void ConfigurationWindow::onDraw() {
  ImGui::BeginChild("ConfigScrollRegion", ImVec2(0, 0), 0,
                    ImGuiWindowFlags_AlwaysVerticalScrollbar);

  drawAssetManagementSection();
  drawSceneParamsSection();
  drawLightingSection();
  drawMaterialCameraLightSection();
  drawAnimationSection();
  drawPrimitivesSection();
  drawEnvironmentSection();
  drawMaterialPreviewSection();

  ImGui::EndChild();
}

// ─────────────────────────────────────────────────────────────────────────────
// Asset management
// ─────────────────────────────────────────────────────────────────────────────

void ConfigurationWindow::drawAssetManagementSection() {
  if (!assetController_) return;

  if (!ImGui::CollapsingHeader("GLTF Assets")) return;

  ImGui::Spacing();

  if (ImGui::Button("Load GLTF Asset...", ImVec2(-FLT_MIN, 0)))
    assetController_->loadAssetFromFile();

  ImGui::Spacing();
  ImGui::Separator();
  ImGui::Spacing();

  auto assets = assetController_->getLoadedAssets();
  auto current_id = assetController_->getCurrentAssetId();

  std::string preview_name = "None";
  if (current_id) {
    auto a = assetController_->getCurrentAsset();
    if (a) preview_name = std::string{a->getName()};
  }

  if (ImGui::BeginCombo("Current Asset", preview_name.c_str())) {
    for (const auto& asset : assets) {
      if (!asset) continue;
      const auto id = asset->getStorageId().value_or(0);
      const bool is_sel = (current_id && *current_id == id);
      if (ImGui::Selectable(std::string{asset->getName()}.c_str(), is_sel))
        assetController_->setCurrentAsset(id);
      if (is_sel) ImGui::SetItemDefaultFocus();
    }
    ImGui::EndCombo();
  }

  if (current_id) {
    auto asset = assetController_->getCurrentAsset();
    if (asset) {
      ImGui::Spacing();

      if (current_id != lastSelectedAssetId_) {
        std::string name{asset->getName()};
        std::strncpy(assetNameBuffer_, name.c_str(), sizeof(assetNameBuffer_) - 1);
        assetNameBuffer_[sizeof(assetNameBuffer_) - 1] = '\0';
        lastSelectedAssetId_ = current_id;
      }

      if (ImGui::InputText("Name", assetNameBuffer_, sizeof(assetNameBuffer_)))
        asset->setName(assetNameBuffer_);

      if (workflowController_ && !asset->gltfMaterials.empty()) {
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::TextDisabled("%d material(s) available for import",
                            static_cast<int>(asset->gltfMaterials.size()));
        ImGui::Spacing();
        if (ImGui::Button("Import Materials to Workflow", ImVec2(-FLT_MIN, 0)))
          workflowController_->importAssetMaterials(asset->gltfMaterials);
      }

      ImGui::Spacing();

      ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6F, 0.2F, 0.2F, 1.0F));
      ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8F, 0.3F, 0.3F, 1.0F));
      ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.9F, 0.4F, 0.4F, 1.0F));
      if (ImGui::Button("Delete Selected Asset", ImVec2(-FLT_MIN, 0))) {
        assetController_->removeAsset(*current_id);
        if (sceneController_) sceneController_->clearSelection();
        lastSelectedAssetId_ = std::nullopt;
      }
      ImGui::PopStyleColor(3);
    }
  } else {
    lastSelectedAssetId_ = std::nullopt;
  }

  ImGui::Spacing();
}

// ─────────────────────────────────────────────────────────────────────────────
// Scene params
// ─────────────────────────────────────────────────────────────────────────────

void ConfigurationWindow::drawSceneParamsSection() {
  if (!sceneController_) return;

  if (!ImGui::CollapsingHeader("Scene Params")) return;

  auto& params = sceneController_->getSceneParams();
  ImGui::Spacing();
  ImGui::SliderFloat("Exposure", &params.exposure, 0.1F, 10.0F);
  ImGui::SliderFloat("Gamma",    &params.gamma,    0.1F,  3.0F);
  ImGui::Spacing();
}

// ─────────────────────────────────────────────────────────────────────────────
// Lighting
// ─────────────────────────────────────────────────────────────────────────────

void ConfigurationWindow::drawLightingSection() {
  if (!sceneController_) return;
  if (!ImGui::CollapsingHeader("Lighting")) return;

  ImGui::Spacing();

  auto& params = sceneController_->getSceneParams();
  bool shadows_on = (params.shadowsEnabled != 0);
  if (ImGui::Checkbox("Enable Shadows", &shadows_on))
    params.shadowsEnabled = shadows_on ? 1 : 0;
  if (shadows_on)
    ImGui::SliderFloat("Shadow Bias", &params.shadowBias, 0.0001F, 0.05F, "%.4f");

  // ── Camera Light ────────────────────────────────────────────────────────────
  ImGui::Spacing();
  ImGui::Separator();
  ImGui::Spacing();

  bool cam_on = sceneController_->isCameraLightEnabled();
  if (ImGui::Checkbox("Camera Light", &cam_on))
    sceneController_->setCameraLightEnabled(cam_on);

  if (cam_on) {
    auto& cl = sceneController_->getCameraLight();
    ImGui::PushID("cam_light");
    ImGui::ColorEdit3("Color##cl",      &cl.color.x);
    ImGui::SliderFloat("Intensity##cl", &cl.intensity, 0.0F, 20.0F);
    ImGui::SliderFloat("Range##cl",     &cl.range,     0.1F, 200.0F);
    bool cl_casts = (cl.castsShadows != 0);
    if (ImGui::Checkbox("Casts Shadows##cl", &cl_casts))
      cl.castsShadows = cl_casts ? 1 : 0;
    ImGui::PopID();
  }

  ImGui::Spacing();
  ImGui::Separator();

  // ── Scene Lights ────────────────────────────────────────────────────────────
  static const char* kLightTypeNames[] = {"Directional", "Point", "Spot"};

  auto& lights = sceneController_->getLights();
  int remove_idx = -1;

  auto sel_light = sceneController_->getSelectedLight();

  for (int idx = 0; idx < static_cast<int>(lights.size()); ++idx) {
    auto& li = lights[idx];
    ImGui::PushID(idx);

    bool is_light_sel = (sel_light && *sel_light == idx);
    if (ImGui::Checkbox("##sel_light", &is_light_sel))
      sceneController_->setSelectedLight(is_light_sel ? std::optional<int>{idx} : std::nullopt);
    ImGui::SameLine();

    const char* type_name = kLightTypeNames[std::clamp(li.type, 0, 2)];
    std::string header = "Light " + std::to_string(idx) + " (" + type_name + ")";
    if (ImGui::TreeNode(header.c_str())) {
      ImGui::Combo("Type", &li.type, kLightTypeNames, 3);
      ImGui::ColorEdit3("Color", &li.color.x);
      ImGui::SliderFloat("Intensity", &li.intensity, 0.0F, 20.0F);

      if (li.type == static_cast<int>(renderer::types::LightType::kDirectional) ||
          li.type == static_cast<int>(renderer::types::LightType::kSpot)) {
        const glm::vec3 d = glm::normalize(li.direction);
        float pitch = glm::degrees(std::asin(glm::clamp(-d.y, -1.0F, 1.0F)));
        float yaw   = glm::degrees(std::atan2(d.x, -d.z));
        bool dir_changed = false;
        if (ImGui::DragFloat("Pitch", &pitch, 0.5F, -90.0F,  90.0F,  "%.1f°")) dir_changed = true;
        if (ImGui::DragFloat("Yaw",   &yaw,   0.5F, -180.0F, 180.0F, "%.1f°")) dir_changed = true;
        if (dir_changed) {
          const float p = glm::radians(pitch), y = glm::radians(yaw);
          li.direction = glm::normalize(glm::vec3(
              std::cos(p) * std::sin(y), -std::sin(p), -std::cos(p) * std::cos(y)));
        }
      }

      if (li.type == static_cast<int>(renderer::types::LightType::kPoint) ||
          li.type == static_cast<int>(renderer::types::LightType::kSpot)) {
        ImGui::DragFloat3("Position", &li.position.x, 0.1F);
        ImGui::SliderFloat("Range", &li.range, 0.1F, 200.0F);
      }
      if (li.type == static_cast<int>(renderer::types::LightType::kSpot)) {
        ImGui::SliderFloat("Inner Angle", &li.innerAngle, 0.01F, 1.57F);
        ImGui::SliderFloat("Outer Angle", &li.outerAngle, 0.01F, 1.57F);
      }

      bool casts = (li.castsShadows != 0);
      if (ImGui::Checkbox("Casts Shadows", &casts))
        li.castsShadows = casts ? 1 : 0;

      if (ImGui::Button("Remove")) remove_idx = idx;
      ImGui::TreePop();
    }
    ImGui::PopID();
  }

  if (remove_idx >= 0) {
    sceneController_->removeLight(remove_idx);
    // Clear selected light if it was removed or is now out of range.
    auto sel = sceneController_->getSelectedLight();
    if (sel && (*sel >= static_cast<int>(lights.size()) || *sel == remove_idx))
      sceneController_->setSelectedLight(std::nullopt);
  }

  ImGui::Spacing();
  if (static_cast<std::uint32_t>(lights.size()) < renderer::types::kMaxSceneLights) {
    if (ImGui::Button("Add Light")) sceneController_->addLight();
  }

  ImGui::Spacing();
}

// ─────────────────────────────────────────────────────────────────────────────
// Material camera light
// ─────────────────────────────────────────────────────────────────────────────

void ConfigurationWindow::drawMaterialCameraLightSection() {
  if (!sceneController_) return;
  if (!ImGui::CollapsingHeader("Material Camera Light")) return;

  ImGui::Spacing();

  bool enabled = sceneController_->isMaterialCameraLightEnabled();
  if (ImGui::Checkbox("Enabled", &enabled))
    sceneController_->setMaterialCameraLightEnabled(enabled);

  if (enabled) {
    auto& li = sceneController_->getMaterialCameraLight();
    ImGui::PushID("mat_cam_light");
    ImGui::ColorEdit3("Color",     &li.color.x);
    ImGui::SliderFloat("Intensity", &li.intensity, 0.0F, 20.0F);
    ImGui::SliderFloat("Range",     &li.range,     0.1F, 200.0F);
    ImGui::PopID();
  }

  ImGui::Spacing();
}

// ─────────────────────────────────────────────────────────────────────────────
// Animations (+ per-asset skinning toggle)
// ─────────────────────────────────────────────────────────────────────────────

void ConfigurationWindow::drawAnimationSection() {
  if (!assetController_) return;

  auto asset = assetController_->getCurrentAsset();
  if (!asset) return;

  const bool has_skins = (asset->skins.getActiveCount() > 0);
  const bool has_anims = !asset->animations.empty();
  if (!has_skins && !has_anims) return;

  if (!ImGui::CollapsingHeader("Animation")) return;

  ImGui::Spacing();

  // Skinning toggle — only shown when the asset has skinned meshes
  if (has_skins && enableSkinning_ && animator_) {
    if (ImGui::Checkbox("Enable Skinning", enableSkinning_)) {
      if (*enableSkinning_)
        animator_->snapshotBindPose(asset.get());
      else
        animator_->resetToBindPose(asset.get());
    }
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
  }

  if (!has_anims || !animator_ || !enableSkinning_ || !*enableSkinning_) {
    ImGui::Spacing();
    return;
  }

  static int selected_anim = 0;
  if (selected_anim >= static_cast<int>(asset->animations.size())) {
    selected_anim = 0;
    animator_->setActiveAnimation(selected_anim);
  }

  const auto preview_name = std::string{asset->animations[selected_anim]->getName()};
  if (ImGui::BeginCombo("Clip", preview_name.c_str())) {
    for (int i = 0; i < static_cast<int>(asset->animations.size()); ++i) {
      const bool is_sel = (selected_anim == i);
      const auto name = std::string{asset->animations[i]->getName()};
      if (ImGui::Selectable(name.c_str(), is_sel)) {
        selected_anim = i;
        animator_->setActiveAnimation(selected_anim);
        animator_->stop();
      }
      if (is_sel) ImGui::SetItemDefaultFocus();
    }
    ImGui::EndCombo();
  }

  ImGui::Spacing();

  bool is_playing = animator_->isPlaying();
  if (ImGui::Button(is_playing ? "Pause" : "Play ", ImVec2(80, 0)))
    animator_->togglePlayback();
  ImGui::SameLine();
  if (ImGui::Button("Stop", ImVec2(80, 0))) animator_->stop();

  float speed = animator_->getTimeScale();
  if (ImGui::SliderFloat("Speed", &speed, 0.0F, 10.0F, "%.2fx"))
    animator_->setTimeScale(speed);

  ImGui::Spacing();
}

// ─────────────────────────────────────────────────────────────────────────────
// Primitives — visibility toggles, multi-select, material slot assignment
// ─────────────────────────────────────────────────────────────────────────────

void ConfigurationWindow::drawPrimitivesSection() {
  if (!assetController_) return;
  auto asset = assetController_->getCurrentAsset();
  if (!asset) return;

  if (!ImGui::CollapsingHeader("Primitives")) return;

  ImGui::Spacing();

  // ── Selection summary ────────────────────────────────────────────────────────
  if (sceneController_) {
    const auto& sel = sceneController_->getSelection();
    if (ImGui::Button("Select All")) {
      for (const auto& mesh : asset->meshes.getItems()) {
        if (!mesh || !mesh->visible) continue;
        for (const auto& prim : mesh->getPrimitives()) {
          if (prim && prim->visible && prim->getStorageId().has_value())
            sceneController_->selectPrimitive(prim->getStorageId().value());
        }
      }
    }
    if (!sel.empty()) {
      ImGui::SameLine();
      ImGui::TextDisabled("%zu selected", sel.size());
      ImGui::SameLine();
      if (ImGui::Button("Clear All")) sceneController_->clearSelection();
    }
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
  }

  // Build mesh-storage-id → node map for TRS controls (first node wins for
  // shared meshes, which is an uncommon glTF instancing case).
  std::unordered_map<std::uint32_t, scene::Node*> mesh_to_node;
  if (!asset->scenes.empty()) {
    auto active_scene = asset->getActiveScene();
    if (active_scene) {
      auto fill_map = [&](auto& self, scene::Node* node) -> void {
        if (node->mesh && node->mesh->getStorageId().has_value())
          mesh_to_node.emplace(node->mesh->getStorageId().value(), node);
        for (const auto& child : node->getChildren())
          self(self, child.get());
      };
      for (std::uint32_t root_id : active_scene->rootNodes)
        if (auto node = asset->nodes.get(root_id))
          fill_map(fill_map, node.get());
    }
  }

  auto meshes = asset->meshes.getItems();
  if (meshes.empty()) {
    ImGui::TextDisabled("No meshes.");
    ImGui::Spacing();
    return;
  }

  // Material slot helpers
  static constexpr std::uint32_t kNoSlot = std::numeric_limits<std::uint32_t>::max();

  auto slot_label = [&](std::uint32_t slotId) -> std::string {
    if (!matManager_) return "Slot " + std::to_string(slotId);
    auto s = matManager_->getSlot(slotId);
    if (!s) return "Slot " + std::to_string(slotId) + " (unresolved)";
    const char* type_name = "None";
    switch (s->getMaterialType()) {
      case material::Type::kDiffuse:        type_name = "Diffuse"; break;
      case material::Type::kDiffuseSpecular: type_name = "Diffuse+Spec"; break;
      case material::Type::kPrincipledBSDF: type_name = "PrincipledBSDF"; break;
      default: break;
    }
    return "Slot " + std::to_string(slotId) + " (" + type_name + ")";
  };

  auto slots = matManager_ ? matManager_->getSlots() : std::vector<std::shared_ptr<material::Slot>>{};

  int mesh_idx = 0;
  for (const auto& mesh : meshes) {
    if (!mesh) { ++mesh_idx; continue; }

    const auto& prims = mesh->getPrimitives();
    if (prims.empty()) { ++mesh_idx; continue; }

    ImGui::PushID(mesh_idx);

    // Mesh row: visibility toggle + tree node
    bool mesh_vis = mesh->visible;
    if (ImGui::Checkbox("##mv", &mesh_vis)) mesh->visible = mesh_vis;
    ImGui::SameLine();

    std::string mesh_label = std::string{mesh->getName()};
    if (mesh_label.empty()) mesh_label = "Mesh " + std::to_string(mesh_idx);
    mesh_label += "  (" + std::to_string(prims.size()) + " prim(s))";

    bool tree_open = ImGui::TreeNodeEx(mesh_label.c_str(),
                                       ImGuiTreeNodeFlags_SpanAvailWidth);
    if (tree_open) {
      ImGui::Spacing();

      // ── Node transform ────────────────────────────────────────────────────
      if (mesh->getStorageId().has_value()) {
        auto it = mesh_to_node.find(mesh->getStorageId().value());
        if (it != mesh_to_node.end()) {
          scene::Node* trs_node = it->second;
          const auto local = trs_node->getLocalTransform();
          glm::vec3 t     = local.getTranslation();
          glm::vec3 euler = glm::degrees(glm::eulerAngles(local.getRotation()));
          glm::vec3 s     = local.getScale();

          ImGui::PushID("trs");
          bool changed = false;
          if (ImGui::DragFloat3("Location", &t.x,     0.01F,  0.0F, 0.0F, "%.4f")) changed = true;
          if (ImGui::DragFloat3("Rotation", &euler.x, 0.1F,   0.0F, 0.0F, "%.2f°")) changed = true;
          if (ImGui::DragFloat3("Scale",    &s.x,     0.005F, 0.0F, 0.0F, "%.4f")) changed = true;
          if (changed)
            trs_node->setLocalTransform(scene::TrsTransform(
                t, glm::normalize(glm::quat(glm::radians(euler))), s));
          ImGui::PopID();
          ImGui::Spacing();
          ImGui::Separator();
          ImGui::Spacing();
        }
      }

      int prim_idx = 0;
      for (const auto& prim : prims) {
        if (!prim) { ++prim_idx; continue; }

        ImGui::PushID(prim_idx);

        const bool has_id = prim->getStorageId().has_value();
        const std::uint32_t storage_id = has_id ? prim->getStorageId().value() : 0;
        const bool is_selected = sceneController_ && has_id &&
                                 sceneController_->isSelected(storage_id);

        // Primitive visibility
        bool prim_vis = prim->visible;
        if (ImGui::Checkbox("##pv", &prim_vis)) {
          prim->visible = prim_vis;
          if (!prim_vis && is_selected && sceneController_)
            sceneController_->deselectPrimitive(storage_id);
        }
        ImGui::SameLine();

        // Select / Deselect checkbox
        if (sceneController_ && has_id) {
          bool sel_state = is_selected;
          if (ImGui::Checkbox("##sel_prim", &sel_state))
            sceneController_->togglePrimitive(storage_id);
          ImGui::SameLine();
        }

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(("Prim " + std::to_string(prim_idx)).c_str());

        // Material slot combo
        if (matManager_) {
          ImGui::SameLine();
          ImGui::SetNextItemWidth(-FLT_MIN);
          const std::uint32_t cur_slot = prim->getMaterialSlot();
          const std::string preview = (cur_slot == kNoSlot) ? "-- None --" : slot_label(cur_slot);

          if (ImGui::BeginCombo("##ms", preview.c_str())) {
            if (ImGui::Selectable("-- None --", cur_slot == kNoSlot))
              prim->setMaterialSlot(kNoSlot);
            if (cur_slot == kNoSlot) ImGui::SetItemDefaultFocus();
            ImGui::Separator();
            for (const auto& slot : slots) {
              if (!slot || !slot->getStorageId().has_value()) continue;
              const std::uint32_t sid = slot->getStorageId().value();
              const std::string label = slot_label(sid);
              const bool is_sel = (cur_slot == sid);
              if (ImGui::Selectable(label.c_str(), is_sel))
                prim->setMaterialSlot(sid);
              if (is_sel) ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
          }
        }

        ImGui::PopID();
        ++prim_idx;
      }

      ImGui::Spacing();
      ImGui::TreePop();
    }

    ImGui::PopID();
    ++mesh_idx;
  }

  ImGui::Spacing();
}

// ─────────────────────────────────────────────────────────────────────────────
// Environment
// ─────────────────────────────────────────────────────────────────────────────

void ConfigurationWindow::drawEnvironmentSection() {
  if (!envController_) return;
  if (!ImGui::CollapsingHeader("Environments")) return;

  ImGui::Spacing();
  if (ImGui::Button("Load Environment Map...", ImVec2(-FLT_MIN, 0)))
    envController_->loadEnvironmentFromFile();

  ImGui::Spacing();
  ImGui::Separator();
  ImGui::Spacing();

  auto environments = envController_->getLoadedEnvironments();
  auto current_id = envController_->getCurrentEnvironmentId();

  std::string preview_name = "None";
  if (current_id) {
    auto e = envController_->getCurrentEnvironment();
    if (e) preview_name = std::string{e->getName()};
  }

  if (ImGui::BeginCombo("Current Env", preview_name.c_str())) {
    for (const auto& env : environments) {
      if (!env) continue;
      const auto id = env->getStorageId().value_or(0);
      const bool is_sel = (current_id && *current_id == id);
      if (ImGui::Selectable(std::string{env->getName()}.c_str(), is_sel))
        envController_->setCurrentEnvironment(id);
      if (is_sel) ImGui::SetItemDefaultFocus();
    }
    ImGui::EndCombo();
  }

  if (current_id) {
    auto env = envController_->getCurrentEnvironment();
    if (env) {
      ImGui::Spacing();

      if (current_id != lastSelectedEnvId_) {
        std::string name{env->getName()};
        std::strncpy(envNameBuffer_, name.c_str(), sizeof(envNameBuffer_) - 1);
        envNameBuffer_[sizeof(envNameBuffer_) - 1] = '\0';
        lastSelectedEnvId_ = current_id;
      }

      if (ImGui::InputText("Env Name", envNameBuffer_, sizeof(envNameBuffer_)))
        env->setName(envNameBuffer_);

      ImGui::Spacing();
      ImGui::SliderFloat("Intensity", &env->intensity, 0.0F, 10.0F);
      ImGui::SliderFloat("Blur",      &env->blurLevel, 0.0F,  1.0F);

      bool use_diff = (env->features & static_cast<std::uint32_t>(env::EnvironmentFeature::kUseDiffuse)) != 0;
      bool use_spec = (env->features & static_cast<std::uint32_t>(env::EnvironmentFeature::kUseSpecular)) != 0;
      if (ImGui::Checkbox("Use Diffuse IBL",  &use_diff))
        env->features ^= static_cast<std::uint32_t>(env::EnvironmentFeature::kUseDiffuse);
      ImGui::SameLine();
      if (ImGui::Checkbox("Use Specular IBL", &use_spec))
        env->features ^= static_cast<std::uint32_t>(env::EnvironmentFeature::kUseSpecular);

      ImGui::Spacing();
      ImGui::Separator();
      ImGui::Spacing();

      ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6F, 0.2F, 0.2F, 1.0F));
      ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8F, 0.3F, 0.3F, 1.0F));
      ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.9F, 0.4F, 0.4F, 1.0F));
      if (ImGui::Button("Delete Selected Env", ImVec2(-FLT_MIN, 0))) {
        envController_->removeEnvironment(*current_id);
        lastSelectedEnvId_ = std::nullopt;
      }
      ImGui::PopStyleColor(3);
    }
  } else {
    lastSelectedEnvId_ = std::nullopt;
  }

  ImGui::Spacing();
}

// ─────────────────────────────────────────────────────────────────────────────
// Material preview
// ─────────────────────────────────────────────────────────────────────────────

void ConfigurationWindow::drawMaterialPreviewSection() {
  if (!matManager_ || !previewSlot_) return;
  if (!ImGui::CollapsingHeader("Material Preview")) return;

  int current_slot = static_cast<int>(*previewSlot_);
  if (ImGui::InputInt("Preview Slot ID", &current_slot))
    *previewSlot_ = static_cast<std::uint32_t>(std::max(0, current_slot));

  ImGui::Spacing();
  ImGui::TextDisabled("Available Slots:");

  ImGui::BeginChild("SlotList", ImVec2(0, 120), 1);
  auto slots = matManager_->getSlots();
  if (slots.empty()) {
    ImGui::TextDisabled("No slots created yet.");
  } else {
    for (const auto& slot : slots) {
      if (!slot || !slot->getStorageId().has_value()) continue;
      std::uint32_t id = slot->getStorageId().value();
      char label[64];
      snprintf(label, sizeof(label), "Slot %u", id);
      bool is_sel = (*previewSlot_ == id);
      if (ImGui::Selectable(label, is_sel)) *previewSlot_ = id;
      if (is_sel) ImGui::SetItemDefaultFocus();
    }
  }
  ImGui::EndChild();
}

};  // namespace vkit::imgui::windows
