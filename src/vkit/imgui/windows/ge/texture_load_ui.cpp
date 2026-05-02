#include "vkit/imgui/windows/ge/texture_load_ui.hpp"

#include <cstring>

#include "vkit/workflow/node/texture_load.hpp"

namespace vkit::imgui::windows::ge {

void TextureLoadNodeUI::drawCanvas(workflow::WorkflowNode* node) {
  auto* tex_node = static_cast<workflow::node::TextureLoadNode*>(node);

  ed::BeginNode(tex_node->getId());

  ImGui::Text("Texture Load");
  ImGui::Dummy(ImVec2(0, 4));

  for (auto& pin : tex_node->getOutputs()) {
    ed::BeginPin(pin->getId(), ed::PinKind::Output);
    ImGui::Text("Color Out ->");
    ed::EndPin();
  }

  ed::EndNode();
}

void TextureLoadNodeUI::drawInspector(workflow::WorkflowNode* node) {
  auto* tex_node = static_cast<workflow::node::TextureLoadNode*>(node);

  ImGui::TextDisabled("Type: Texture Load Node");
  ImGui::Separator();

  std::string current_path = tex_node->getPath().string();

  char buf[512];
  std::strncpy(buf, current_path.c_str(), sizeof(buf));
  buf[sizeof(buf) - 1] = '\0';

  if (ImGui::InputText("File Path", buf, sizeof(buf))) {
    tex_node->setPath(buf);
  }

  ImGui::Spacing();

  if (tex_node->status() == workflow::NodeStatus::kExecuting) {
    ImGui::TextColored(ImVec4(1, 1, 0, 1), "Loading from disk...");
  } else if (tex_node->isLoaded()) {
    ImGui::TextColored(ImVec4(0, 1, 0, 1), "Loaded successfully!");

    if (tex_node->outputTextureId.has_value() && textureManager_) {
      auto tex = textureManager_->get(tex_node->outputTextureId.value());
      if (tex && tex->getImguiId().has_value()) {
        if (tex->getImguiId().has_value()) {
          ImGui::Image(*tex->getImguiId(), ImVec2(256, 256));
        }
      }
    }
  }
}

};  // namespace vkit::imgui::windows::ge
