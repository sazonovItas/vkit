#include "vkit/imgui/windows/ge/node/normalmap_ui.hpp"

#include <imgui.h>
#include <imnodes.h>

#include "vkit/controller/workflow.hpp"
#include "vkit/imgui/windows/ge/pin_ui.hpp"
#include "vkit/imgui/windows/ge/style.hpp"
#include "vkit/workflow/node/operators/normalmap.hpp"

namespace vkit::imgui::windows::ge {

using workflow::node::op::NormalMapNode;
using workflow::node::op::NormalMapParams;

auto NormalMapNodeUI::spawnNode(controller::WorkflowController* controller)
    -> workflow::WorkflowNode* {
  return controller->createNormalMapNode("Normal Map");
}

void NormalMapNodeUI::drawCanvas(workflow::WorkflowNode* node) {
  auto* n = static_cast<NormalMapNode*>(node);
  ImNodes::PushColorStyle(
      ImNodesCol_TitleBar,
      ImGui::ColorConvertFloat4ToU32(getStatusColor(n->status())));
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
  ImGui::TextDisabled("Strength: %.2f", p.strength);

  ImGui::Dummy(ImVec2(node_width, 2.0F * zoom));

  PinUI::DrawInput(n->getInputs()[0].get(), "Height Map",
                   style::colors::kPinColorYellow);
  ImGui::Dummy(ImVec2(0.0f, ImGui::GetStyle().ItemSpacing.y * zoom));

  PinUI::DrawOutput(n->getOutputs()[0].get(), "Normal (F32)",
                    style::colors::kPinColorCyan, node_width);
  ImGui::Dummy(ImVec2(0.0f, ImGui::GetStyle().ItemSpacing.y * zoom));
  PinUI::DrawOutput(n->getOutputs()[1].get(), "Normal (Color)",
                    style::colors::kPinColorYellow, node_width);
  ImGui::Dummy(ImVec2(0.0f, ImGui::GetStyle().ItemSpacing.y * zoom));

  ImNodes::EndNode();

  ImGui::PopStyleColor();
  ImNodes::PopColorStyle();
  ImNodes::PopColorStyle();
}

void NormalMapNodeUI::drawInspector(workflow::WorkflowNode* node) {
  auto* n = static_cast<NormalMapNode*>(node);
  NormalMapParams p = n->getParams();
  bool changed = false;

  ImGui::TextDisabled("Type: Normal Map");
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

  if (ImGui::DragFloat("Strength", &p.strength, 0.05F, 0.0F, 10.0F))
    changed = true;

  bool invert_x = (p.invertX != 0);
  if (ImGui::Checkbox("Invert X", &invert_x)) {
    p.invertX = invert_x ? 1 : 0;
    changed = true;
  }

  bool invert_y = (p.invertY != 0);
  if (ImGui::Checkbox("Invert Y (DirectX)", &invert_y)) {
    p.invertY = invert_y ? 1 : 0;
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
        ImGui::Image(*tex->getImguiId(), ImVec2(panel_width, panel_width));
      }
    }
  } else if (n->status() == workflow::NodeStatus::kExecuting) {
    ImGui::TextColored(style::colors::kTextExecuting, "Processing...");
  } else if (n->status() == workflow::NodeStatus::kError) {
    ImGui::TextColored(style::colors::kTextError, "Error processing image.");
  }
}

};  // namespace vkit::imgui::windows::ge
