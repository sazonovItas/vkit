#include "vkit/imgui/windows/ge/node/texture_load_ui.hpp"

#include <imgui.h>
#include <imnodes.h>

#include <cstring>
#include <filesystem>

#include "vkit/imgui/windows/ge/pin_ui.hpp"
#include "vkit/imgui/windows/ge/style.hpp"
#include "vkit/platform/file_dialog.hpp"
#include "vkit/workflow/node/texture_load.hpp"

namespace vkit::imgui::windows::ge {

using workflow::node::TextureLoadNode;

void TextureLoadNodeUI::drawCanvas(workflow::WorkflowNode* node) {
  auto* n = static_cast<TextureLoadNode*>(node);

  ImVec4 status_color = getStatusColor(n->status());

  ImU32 header_color = ImGui::ColorConvertFloat4ToU32(status_color);
  ImNodes::PushColorStyle(ImNodesCol_TitleBar, header_color);
  ImNodes::PushColorStyle(
      ImNodesCol_NodeBackground,
      ImGui::ColorConvertFloat4ToU32(style::colors::kNodeBg));
  ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0F, 1.0F, 1.0F, 1.0F));

  ImNodes::BeginNode(n->getId());

  ImNodes::BeginNodeTitleBar();
  ImGui::TextUnformatted(n->getName().c_str());
  ImNodes::EndNodeTitleBar();

  const auto zoom = ImNodes::EditorContextGetZoom();
  float const node_width = 180.0F * zoom;
  ImGui::Dummy(ImVec2(node_width, 4.0F * zoom));

  PinUI::DrawOutput(n->getOutputs()[0].get(), "Image",
                    style::colors::kPinColorCyan, node_width);
  ImGui::Spacing();
  PinUI::DrawOutput(n->getOutputs()[1].get(), "Color",
                    style::colors::kPinColorYellow, node_width);
  ImGui::Spacing();

  ImNodes::EndNode();

  ImGui::PopStyleColor();
  ImNodes::PopColorStyle();
  ImNodes::PopColorStyle();
}

void TextureLoadNodeUI::drawInspector(workflow::WorkflowNode* node) {
  auto* n = static_cast<TextureLoadNode*>(node);

  ImGui::TextDisabled("Type: Texture Load Node");
  ImGui::Separator();

  char name_buf[256];
  std::strncpy(name_buf, n->getName().c_str(), sizeof(name_buf));
  name_buf[sizeof(name_buf) - 1] = '\0';

  if (ImGui::InputText("Node Name", name_buf, sizeof(name_buf))) {
    n->setName(name_buf);
  }

  ImGui::Spacing();
  ImGui::Separator();
  ImGui::Spacing();

  bool use_mipmaps = n->getUseMipmaps();
  if (ImGui::Checkbox("Generate Mipmaps", &use_mipmaps)) {
    n->setUseMipmaps(use_mipmaps);
  }
  ImGui::Spacing();

  std::string current_path = n->getPath().string();
  char buf[512];
  std::strncpy(buf, current_path.c_str(), sizeof(buf));
  buf[sizeof(buf) - 1] = '\0';

  if (ImGui::InputText("##path", buf, sizeof(buf))) {
    n->setPath(buf);
  }
  ImGui::SameLine();
  if (ImGui::Button("Browse...")) {
    static constexpr platform::FileFilter kImageFilters[] = {
        {.name = "Images", .spec = "png,jpg,jpeg,hdr,tga,bmp"},
    };
    if (auto path = platform::openFileDialog(kImageFilters)) {
      n->setPath(*path);
    }
  }

  std::error_code ec;
  if (!current_path.empty() && !std::filesystem::exists(current_path, ec)) {
    ImGui::TextColored(style::colors::kTextWarning,
                       "Warning: File does not exist!");
  }

  ImGui::Spacing();

  if (n->status() == workflow::NodeStatus::kExecuting) {
    ImGui::TextColored(style::colors::kTextExecuting, "Loading from disk...");
  } else if (n->status() == workflow::NodeStatus::kError) {
    ImGui::TextColored(style::colors::kTextError,
                       "Error: Failed to load texture.");
  } else if (n->isLoaded()) {
    ImGui::TextColored(style::colors::kTextSuccess, "Loaded successfully!");

    if (n->outputColorId.has_value() && textureManager_) {
      auto tex = textureManager_->get(n->outputColorId.value());
      if (tex && tex->getImguiId().has_value()) {
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        ImGui::TextDisabled("Preview:");

        float panel_width = ImGui::GetContentRegionAvail().x;
        ImVec2 image_size(panel_width, panel_width);

        ImGui::Image(*tex->getImguiId(), image_size);
      }
    }
  }
}

};  // namespace vkit::imgui::windows::ge
