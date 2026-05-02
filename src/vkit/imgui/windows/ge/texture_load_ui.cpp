#include "vkit/imgui/windows/ge/texture_load_ui.hpp"

#include <imgui.h>
#include <imnodes.h>

#include <cstring>
#include <filesystem>

#include "vkit/imgui/windows/ge/pin_ui.hpp"
#include "vkit/imgui/windows/ge/style.hpp"
#include "vkit/platform/file_dialog.hpp"
#include "vkit/workflow/node/texture_load.hpp"

namespace vkit::imgui::windows::ge {

void TextureLoadNodeUI::drawCanvas(workflow::WorkflowNode* node) {
  auto* tex_node = static_cast<workflow::node::TextureLoadNode*>(node);

  ImVec4 status_color = style::colors::kStatusDefault;
  switch (tex_node->status()) {
    case workflow::NodeStatus::kReady:
      status_color = style::colors::kStatusReady;
      break;
    case workflow::NodeStatus::kStale:
      status_color = style::colors::kStatusStale;
      break;
    case workflow::NodeStatus::kExecuting:
      status_color = style::colors::kStatusExecuting;
      break;
    case workflow::NodeStatus::kError:
      status_color = style::colors::kStatusError;
      break;
  }

  ImU32 header_color = ImGui::ColorConvertFloat4ToU32(status_color);
  ImU32 header_hover = ImGui::ColorConvertFloat4ToU32(
      ImVec4(status_color.x * 1.2F, status_color.y * 1.2F,
             status_color.z * 1.2F, 1.0F));

  ImNodes::PushColorStyle(ImNodesCol_TitleBar, IM_COL32(255, 255, 255, 255));
  ImNodes::PushColorStyle(ImNodesCol_TitleBar, header_color);
  ImNodes::PushColorStyle(
      ImNodesCol_NodeBackground,
      ImGui::ColorConvertFloat4ToU32(style::colors::kNodeBg));

  ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0F, 1.0F, 1.0F, 1.0F));

  ImNodes::BeginNode(tex_node->getId());

  ImNodes::BeginNodeTitleBar();
  ImGui::TextUnformatted(tex_node->getName().c_str());
  ImNodes::EndNodeTitleBar();

  float const node_width = 180.0F * ImNodes::EditorContextGetZoom();
  ImGui::Dummy(ImVec2(node_width, 4.0F * ImNodes::EditorContextGetZoom()));

  for (auto& pin : tex_node->getOutputs()) {
    PinUI::DrawOutput(pin.get(), "Color", style::colors::kPinColorYellow,
                      node_width);
  }

  ImNodes::EndNode();

  ImGui::PopStyleColor();
  ImNodes::PopColorStyle();
  ImNodes::PopColorStyle();
  ImNodes::PopColorStyle();
}

void TextureLoadNodeUI::drawInspector(workflow::WorkflowNode* node) {
  auto* tex_node = static_cast<workflow::node::TextureLoadNode*>(node);

  ImGui::TextDisabled("Type: Texture Load Node");
  ImGui::Separator();

  char name_buf[256];
  std::strncpy(name_buf, tex_node->getName().c_str(), sizeof(name_buf));
  name_buf[sizeof(name_buf) - 1] = '\0';

  if (ImGui::InputText("Node Name", name_buf, sizeof(name_buf))) {
    tex_node->setName(name_buf);
  }

  ImGui::Spacing();
  ImGui::Separator();
  ImGui::Spacing();

  std::string current_path = tex_node->getPath().string();
  char buf[512];
  std::strncpy(buf, current_path.c_str(), sizeof(buf));
  buf[sizeof(buf) - 1] = '\0';

  if (ImGui::InputText("##path", buf, sizeof(buf))) {
    tex_node->setPath(buf);
  }
  ImGui::SameLine();
  if (ImGui::Button("Browse...")) {
    static constexpr platform::FileFilter kImageFilters[] = {
        {.name = "Images", .spec = "png,jpg,jpeg,hdr,tga,bmp"},
    };
    if (auto path = platform::openFileDialog(kImageFilters)) {
      tex_node->setPath(*path);
    }
  }

  std::error_code ec;
  if (!current_path.empty() && !std::filesystem::exists(current_path, ec)) {
    ImGui::TextColored(style::colors::kTextWarning,
                       "Warning: File does not exist!");
  }

  ImGui::Spacing();

  if (tex_node->status() == workflow::NodeStatus::kExecuting) {
    ImGui::TextColored(style::colors::kTextExecuting, "Loading from disk...");
  } else if (tex_node->status() == workflow::NodeStatus::kError) {
    ImGui::TextColored(style::colors::kTextError,
                       "Error: Failed to load texture.");
  } else if (tex_node->isLoaded()) {
    ImGui::TextColored(style::colors::kTextSuccess, "Loaded successfully!");

    if (tex_node->outputTextureId.has_value() && textureManager_) {
      auto tex = textureManager_->get(tex_node->outputTextureId.value());
      if (tex && tex->getImguiId().has_value()) {
        ImGui::Spacing();
        ImGui::Image(*tex->getImguiId(), ImVec2(256, 256));
      }
    }
  }
}

};  // namespace vkit::imgui::windows::ge
