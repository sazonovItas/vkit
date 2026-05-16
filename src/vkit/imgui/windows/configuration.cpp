#include "vkit/imgui/windows/configuration.hpp"

#include <imgui.h>

#include <cstring>
#include <limits>
#include <string>

#include "vkit/animation/animator.hpp"
#include "vkit/controller/asset.hpp"
#include "vkit/controller/environment.hpp"
#include "vkit/controller/workflow.hpp"
#include "vkit/material/manager.hpp"

namespace vkit::imgui::windows {

ConfigurationWindow::ConfigurationWindow(
    std::string_view title, controller::AssetController* assetController,
    controller::EnvironmentController* envController,
    animation::Animator* animator, bool* enableSkinning,
    material::MaterialManager* matManager, std::uint32_t* previewSlot)
    : ImguiWindow(title),
      assetController_{assetController},
      envController_{envController},
      animator_{animator},
      enableSkinning_{enableSkinning},
      matManager_{matManager},
      previewSlot_{previewSlot} {
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

void ConfigurationWindow::setAnimator(animation::Animator* animator) {
  animator_ = animator;
}

void ConfigurationWindow::setEnableSkinning(bool* enableSkinning) {
  enableSkinning_ = enableSkinning;
}

void ConfigurationWindow::setSceneParams(
    renderer::types::SceneParamsUBO* sceneParams) {
  sceneParams_ = sceneParams;
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
  drawSceneParamsConfigurationSection();
  drawAnimationManagementSection();
  drawPrimitiveMaterialSection();
  drawEnvironmentManagementSection();
  drawMaterialPreivewSection();

  ImGui::EndChild();
}

void ConfigurationWindow::drawAssetManagementSection() {
  if (!assetController_) return;

  if (ImGui::CollapsingHeader("GLTF Assets")) {
    ImGui::Spacing();

    if (ImGui::Button("Load GLTF Asset...", ImVec2(-FLT_MIN, 0))) {
      assetController_->loadAssetFromFile();
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    auto assets = assetController_->getLoadedAssets();
    auto current_id = assetController_->getCurrentAssetId();

    std::string preview_name = "None";
    if (current_id.has_value()) {
      auto current_asset = assetController_->getCurrentAsset();
      if (current_asset) {
        preview_name = std::string{current_asset->getName()};
      }
    }

    if (ImGui::BeginCombo("Current Asset", preview_name.c_str())) {
      for (const auto& asset : assets) {
        if (!asset) continue;

        const auto asset_id = asset->getStorageId().value_or(0);
        const bool is_selected =
            (current_id.has_value() && current_id.value() == asset_id);

        std::string asset_name{asset->getName()};

        if (ImGui::Selectable(asset_name.c_str(), is_selected)) {
          assetController_->setCurrentAsset(asset_id);
        }

        if (is_selected) {
          ImGui::SetItemDefaultFocus();
        }
      }
      ImGui::EndCombo();
    }

    if (current_id.has_value()) {
      auto current_asset = assetController_->getCurrentAsset();
      if (current_asset) {
        ImGui::Spacing();

        if (current_id != lastSelectedAssetId_) {
          std::string name_str{current_asset->getName()};
          std::strncpy(assetNameBuffer_, name_str.c_str(),
                       sizeof(assetNameBuffer_) - 1);
          assetNameBuffer_[sizeof(assetNameBuffer_) - 1] = '\0';

          lastSelectedAssetId_ = current_id;
        }

        if (ImGui::InputText("Name", assetNameBuffer_,
                             sizeof(assetNameBuffer_))) {
          current_asset->setName(assetNameBuffer_);
        }

        if (workflowController_ && !current_asset->gltfMaterials.empty()) {
          ImGui::Spacing();
          ImGui::Separator();
          ImGui::Spacing();

          const int mat_count =
              static_cast<int>(current_asset->gltfMaterials.size());
          ImGui::TextDisabled("%d material(s) available for import", mat_count);
          ImGui::Spacing();

          if (ImGui::Button("Import Materials to Workflow",
                            ImVec2(-FLT_MIN, 0))) {
            workflowController_->importAssetMaterials(
                current_asset->gltfMaterials);
          }
        }

        ImGui::Spacing();

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6F, 0.2F, 0.2F, 1.0F));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
                              ImVec4(0.8F, 0.3F, 0.3F, 1.0F));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,
                              ImVec4(0.9F, 0.4F, 0.4F, 1.0F));

        if (ImGui::Button("Delete Selected Asset", ImVec2(-FLT_MIN, 0))) {
          assetController_->removeAsset(current_id.value());
          lastSelectedAssetId_ = std::nullopt;
        }

        ImGui::PopStyleColor(3);
      }
    } else {
      lastSelectedAssetId_ = std::nullopt;
    }

    ImGui::Spacing();
  }
}

void ConfigurationWindow::drawSceneParamsConfigurationSection() {
  if (!sceneParams_) return;

  if (ImGui::CollapsingHeader("Scene Params")) {
    ImGui::Spacing();
    ImGui::SliderFloat("Exposure", &sceneParams_->exposure, 0.1F, 10.0F);
    ImGui::SliderFloat("Gamma",    &sceneParams_->gamma,    0.1F,  3.0F);
  }
}

void ConfigurationWindow::drawAnimationManagementSection() {
  if (!assetController_ || !animator_ || !enableSkinning_) return;

  auto current_asset = assetController_->getCurrentAsset();
  if (!current_asset || current_asset->animations.empty()) return;

  if (ImGui::CollapsingHeader("Animations")) {
    ImGui::Spacing();

    if (ImGui::Checkbox("Enable Skinning & Animation", enableSkinning_)) {
      if (!*enableSkinning_) {
        animator_->stop();
      }
    }

    if (*enableSkinning_) {
      ImGui::Spacing();
      ImGui::Separator();
      ImGui::Spacing();

      static int selected_anim = 0;
      if (selected_anim >= current_asset->animations.size()) {
        selected_anim = 0;
        animator_->setActiveAnimation(selected_anim);
      }

      const auto preview_name =
          std::string{current_asset->animations[selected_anim]->getName()};

      if (ImGui::BeginCombo("Clip", preview_name.c_str())) {
        for (int i = 0; i < current_asset->animations.size(); ++i) {
          bool is_selected = (selected_anim == i);

          const auto item_name =
              std::string{current_asset->animations[i]->getName()};

          if (ImGui::Selectable(item_name.c_str(), is_selected)) {
            selected_anim = i;
            animator_->setActiveAnimation(selected_anim);
            animator_->stop();
            animator_->play();
          }
          if (is_selected) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
      }

      ImGui::Spacing();

      bool is_playing = animator_->isPlaying();
      if (ImGui::Button(is_playing ? "Pause" : "Play ", ImVec2(80, 0))) {
        animator_->togglePlayback();
      }

      ImGui::SameLine();

      if (ImGui::Button("Stop", ImVec2(80, 0))) {
        animator_->stop();
      }

      float speed = animator_->getTimeScale();
      if (ImGui::SliderFloat("Speed", &speed, 0.0F, 10.0F, "%.2fx")) {
        animator_->setTimeScale(speed);
      }
    }

    ImGui::Spacing();
  }
}

void ConfigurationWindow::drawPrimitiveMaterialSection() {
  if (!assetController_ || !matManager_) return;
  auto asset = assetController_->getCurrentAsset();
  if (!asset) return;

  if (!ImGui::CollapsingHeader("Primitive Materials")) return;

  ImGui::Spacing();

  auto slots = matManager_->getSlots();
  auto meshes = asset->meshes.getItems();

  if (meshes.empty()) {
    ImGui::TextDisabled("No meshes in current asset.");
    ImGui::Spacing();
    return;
  }

  if (slots.empty()) {
    ImGui::TextDisabled("No material slots available.");
    ImGui::TextDisabled("Create slots via the workflow graph first.");
    ImGui::Spacing();
  }

  static constexpr std::uint32_t kNoSlot =
      std::numeric_limits<std::uint32_t>::max();

  auto slot_label = [&](std::uint32_t slotId) -> std::string {
    auto s = matManager_->getSlot(slotId);
    if (!s) return "Slot " + std::to_string(slotId) + "  (unresolved)";
    const char* type_name = "None";
    switch (s->getMaterialType()) {
      case material::Type::kDiffuse:
        type_name = "Diffuse";
        break;
      case material::Type::kDiffuseSpecular:
        type_name = "Diffuse Specular";
        break;
      case material::Type::kPrincipledBSDF:
        type_name = "Principled BSDF";
        break;
      default:
        type_name = "None";
        break;
    }
    return "Slot " + std::to_string(slotId) + "  (" + type_name + ")";
  };

  int mesh_idx = 0;
  for (const auto& mesh : meshes) {
    if (!mesh) {
      ++mesh_idx;
      continue;
    }

    const auto& prims = mesh->getPrimitives();
    if (prims.empty()) {
      ++mesh_idx;
      continue;
    }

    std::string mesh_name{mesh->getName()};
    if (mesh_name.empty()) mesh_name = "Mesh " + std::to_string(mesh_idx);
    mesh_name += "  (" + std::to_string(prims.size()) + " primitives)";

    ImGui::PushID(mesh_idx);
    if (ImGui::TreeNodeEx(mesh_name.c_str(), ImGuiTreeNodeFlags_None)) {
      ImGui::Spacing();

      int prim_idx = 0;
      for (const auto& prim : prims) {
        if (!prim) {
          ++prim_idx;
          continue;
        }

        std::uint32_t cur_slot = prim->getMaterialSlot();

        std::string preview =
            (cur_slot == kNoSlot) ? "-- None --" : slot_label(cur_slot);

        ImGui::PushID(prim_idx);

        ImGui::AlignTextToFramePadding();
        ImGui::TextDisabled("Primitive %d", prim_idx);
        ImGui::SameLine();
        ImGui::SetNextItemWidth(-FLT_MIN);

        if (ImGui::BeginCombo("##matslot", preview.c_str())) {
          {
            const bool is_none = (cur_slot == kNoSlot);
            if (ImGui::Selectable("-- None --", is_none))
              prim->setMaterialSlot(kNoSlot);
            if (is_none) ImGui::SetItemDefaultFocus();
          }

          ImGui::Separator();

          for (const auto& slot : slots) {
            if (!slot || !slot->getStorageId().has_value()) continue;
            const std::uint32_t slot_id = slot->getStorageId().value();
            const std::string label = slot_label(slot_id);
            const bool is_sel = (cur_slot == slot_id);
            if (ImGui::Selectable(label.c_str(), is_sel))
              prim->setMaterialSlot(slot_id);
            if (is_sel) ImGui::SetItemDefaultFocus();
          }

          ImGui::EndCombo();
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

void ConfigurationWindow::drawEnvironmentManagementSection() {
  if (!envController_) return;

  if (ImGui::CollapsingHeader("Environments")) {
    ImGui::Spacing();

    if (ImGui::Button("Load Environment Map...", ImVec2(-FLT_MIN, 0))) {
      envController_->loadEnvironmentFromFile();
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    auto environments = envController_->getLoadedEnvironments();
    auto current_id = envController_->getCurrentEnvironmentId();

    std::string preview_name = "None";
    if (current_id.has_value()) {
      auto current_env = envController_->getCurrentEnvironment();
      if (current_env) {
        preview_name = std::string{current_env->getName()};
      }
    }

    if (ImGui::BeginCombo("Current Env", preview_name.c_str())) {
      for (const auto& env : environments) {
        if (!env) continue;

        const auto env_id = env->getStorageId().value_or(0);
        const bool is_selected =
            (current_id.has_value() && current_id.value() == env_id);

        std::string env_name{env->getName()};

        if (ImGui::Selectable(env_name.c_str(), is_selected)) {
          envController_->setCurrentEnvironment(env_id);
        }

        if (is_selected) ImGui::SetItemDefaultFocus();
      }
      ImGui::EndCombo();
    }

    if (current_id.has_value()) {
      auto current_env = envController_->getCurrentEnvironment();
      if (current_env) {
        ImGui::Spacing();

        if (current_id != lastSelectedEnvId_) {
          std::string name_str{current_env->getName()};
          std::strncpy(envNameBuffer_, name_str.c_str(),
                       sizeof(envNameBuffer_) - 1);
          envNameBuffer_[sizeof(envNameBuffer_) - 1] = '\0';
          lastSelectedEnvId_ = current_id;
        }

        if (ImGui::InputText("Env Name", envNameBuffer_,
                             sizeof(envNameBuffer_))) {
          current_env->setName(envNameBuffer_);
        }

        ImGui::Spacing();
        ImGui::SliderFloat("Intensity", &current_env->intensity, 0.0F, 10.0F);

        ImGui::SliderFloat("Blur", &current_env->blurLevel, 0.0F, 1.0F);

        bool use_diffuse = (current_env->features &
                            static_cast<std::uint32_t>(
                                env::EnvironmentFeature::kUseDiffuse)) != 0;
        bool use_specular = (current_env->features &
                             static_cast<std::uint32_t>(
                                 env::EnvironmentFeature::kUseSpecular)) != 0;

        if (ImGui::Checkbox("Use Diffuse IBL", &use_diffuse)) {
          current_env->features ^=
              static_cast<std::uint32_t>(env::EnvironmentFeature::kUseDiffuse);
        }
        ImGui::SameLine();
        if (ImGui::Checkbox("Use Specular IBL", &use_specular)) {
          current_env->features ^=
              static_cast<std::uint32_t>(env::EnvironmentFeature::kUseSpecular);
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6F, 0.2F, 0.2F, 1.0F));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
                              ImVec4(0.8F, 0.3F, 0.3F, 1.0F));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,
                              ImVec4(0.9F, 0.4F, 0.4F, 1.0F));

        if (ImGui::Button("Delete Selected Env", ImVec2(-FLT_MIN, 0))) {
          envController_->removeEnvironment(current_id.value());
          lastSelectedEnvId_ = std::nullopt;
        }

        ImGui::PopStyleColor(3);
      }
    } else {
      lastSelectedEnvId_ = std::nullopt;
    }

    ImGui::Spacing();
  }
}

void ConfigurationWindow::drawMaterialPreivewSection() {
  if (!matManager_ || !previewSlot_) return;

  if (ImGui::CollapsingHeader("Material Preview")) {
    int current_slot = static_cast<int>(*previewSlot_);
    if (ImGui::InputInt("Preview Slot ID", &current_slot)) {
      *previewSlot_ = static_cast<std::uint32_t>(std::max(0, current_slot));
    }

    ImGui::Spacing();
    ImGui::TextDisabled("Available Active Slots:");

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

        bool is_selected = (*previewSlot_ == id);
        if (ImGui::Selectable(label, is_selected)) {
          *previewSlot_ = id;
        }

        if (is_selected) ImGui::SetItemDefaultFocus();
      }
    }
    ImGui::EndChild();
  }
}

};  // namespace vkit::imgui::windows
