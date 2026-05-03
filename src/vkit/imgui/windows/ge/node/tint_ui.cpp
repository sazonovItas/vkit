#include "vkit/imgui/windows/ge/node/tint_ui.hpp"

#include <imgui.h>
#include <imnodes.h>

#include "vkit/controller/workflow.hpp"
#include "vkit/imgui/windows/ge/pin_ui.hpp"
#include "vkit/imgui/windows/ge/style.hpp"
#include "vkit/workflow/node/tint.hpp"

namespace vkit::imgui::windows::ge {

using workflow::node::TintNode;
using workflow::node::TintParams;

auto TintNodeUI::spawnNode(controller::WorkflowController* controller)
    -> workflow::WorkflowNode* {
  return controller->createTintNode("Color Tint");
}

void TintNodeUI::drawCanvas(workflow::WorkflowNode* node) {
  auto* n = static_cast<TintNode*>(node);

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

  const float zoom = ImNodes::EditorContextGetZoom();
  const float node_width = 180.0F * zoom;
  ImGui::Dummy(ImVec2(node_width, 2.0F * zoom));

  const auto& p = n->getParams();
  ImVec4 display_color = ImVec4(p.color[0], p.color[1], p.color[2], 1.0F);
  ImGui::ColorButton("##tint_color", display_color,
                     ImGuiColorEditFlags_NoTooltip,
                     ImVec2(node_width, 16.0F * zoom));

  ImGui::Dummy(ImVec2(node_width, 2.0F * zoom));

  PinUI::DrawInput(n->getInputs()[0].get(), "Color",
                   style::colors::kPinColorYellow);
  ImGui::Spacing();

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

void TintNodeUI::drawInspector(workflow::WorkflowNode* node) {
  auto* n = static_cast<workflow::node::TintNode*>(node);
  workflow::node::TintParams p = n->getParams();
  bool changed = false;

  ImGui::TextDisabled("Type: Color Tint");
  ImGui::Separator();

  {
    char buf[256];
    std::strncpy(buf, n->getName().c_str(), sizeof(buf));
    buf[sizeof(buf) - 1] = '\0';
    if (ImGui::InputText("Node Name", buf, sizeof(buf))) n->setName(buf);
  }

  ImGui::Spacing();
  ImGui::Separator();
  ImGui::Spacing();

  const char* mode_names[] = {"Mix", "Multiply", "Add", "Screen"};
  int current_mode = static_cast<int>(p.mode);
  if (ImGui::Combo("Blend Mode", &current_mode, mode_names, 4)) {
    p.mode = static_cast<workflow::node::TintMode>(current_mode);
    changed = true;
  }

  if (ImGui::SliderFloat("Factor (Fac)", &p.factor, 0.0F, 1.0F)) {
    changed = true;
  }

  ImGui::Spacing();

  if (ImGui::ColorEdit4("Color", p.color, ImGuiColorEditFlags_Float)) {
    changed = true;
  }

  if (changed) n->setParams(p);

  if (n->status() == workflow::NodeStatus::kReady) {
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
  } else if (n->status() == workflow::NodeStatus::kExecuting) {
    ImGui::TextColored(style::colors::kTextExecuting, "Processing...");
  } else if (n->status() == workflow::NodeStatus::kError) {
    ImGui::TextColored(style::colors::kTextError, "Error processing image.");
  }
}

};  // namespace vkit::imgui::windows::ge
