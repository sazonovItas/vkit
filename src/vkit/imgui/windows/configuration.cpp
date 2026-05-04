#include "vkit/imgui/windows/configuration.hpp"

#include <imgui.h>

#include <cstring>
#include <string>

#include "vkit/animation/animator.hpp"
#include "vkit/controller/asset.hpp"
#include "vkit/controller/environment.hpp"

namespace vkit::imgui::windows {

ConfigurationWindow::ConfigurationWindow(
    std::string_view title, controller::AssetController* assetController,
    controller::EnvironmentController* envController,
    animation::Animator* animator)
    : ImguiWindow(title),
      assetController_{assetController},
      envController_{envController},
      animator_{animator} {
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

void ConfigurationWindow::onDraw() {
  ImGui::BeginChild("ConfigScrollRegion", ImVec2(0, 0), 0,
                    ImGuiWindowFlags_AlwaysVerticalScrollbar);

  drawAssetManagementSection();
  drawAnimationManagementSection();
  drawEnvironmentManagementSection();

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

void ConfigurationWindow::drawAnimationManagementSection() {
  if (!assetController_ || !animator_) return;

  auto current_asset = assetController_->getCurrentAsset();
  if (!current_asset || current_asset->animations.empty()) return;

  if (ImGui::CollapsingHeader("Animations", ImGuiTreeNodeFlags_DefaultOpen)) {
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
        if (ImGui::Selectable(preview_name.c_str(), is_selected)) {
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
    ImGui::Separator();
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

    ImGui::Spacing();
  }
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

};  // namespace vkit::imgui::windows
