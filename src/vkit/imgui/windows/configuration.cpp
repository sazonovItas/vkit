#include "vkit/imgui/windows/configuration.hpp"

#include <imgui.h>

#include <cstring>
#include <string>

#include "vkit/controller/asset.hpp"

namespace vkit::imgui::windows {

ConfigurationWindow::ConfigurationWindow(
    std::string_view title, controller::AssetController* assetController)
    : ImguiWindow(title), assetController_{assetController} {
  setMinSize(250.0F, 200.0F);
}

void ConfigurationWindow::setAssetController(
    controller::AssetController* assetController) {
  assetController_ = assetController;
}

auto ConfigurationWindow::getFlags() -> ImGuiWindowFlags {
  return ImguiWindow::getFlags() | ImGuiWindowFlags_AlwaysVerticalScrollbar;
}

void ConfigurationWindow::onDraw() { drawAssetManagementSection(); }

void ConfigurationWindow::drawAssetManagementSection() {
  if (!assetController_) return;

  if (ImGui::CollapsingHeader("GLTF Assets", ImGuiTreeNodeFlags_DefaultOpen)) {
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

};  // namespace vkit::imgui::windows
