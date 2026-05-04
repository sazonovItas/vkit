#include <imgui.h>
#include <imnodes.h>

#include <algorithm>
#include <cstring>

#include "vkit/controller/workflow.hpp"
#include "vkit/imgui/windows/ge/node/slot_output_ui.hpp"
#include "vkit/imgui/windows/ge/pin_ui.hpp"
#include "vkit/imgui/windows/ge/style.hpp"
#include "vkit/workflow/node/material/slot_output.hpp"

namespace vkit::imgui::windows::ge {

using workflow::node::mat::SlotOutputNode;

const ImVec4 kPinColorMaterial = ImVec4(0.8F, 0.3F, 0.8F, 1.0F);

auto SlotOutputNodeUI::spawnNode(controller::WorkflowController* controller)
    -> workflow::WorkflowNode* {
  return controller->createSlotOutputNode("Material Slot");
}

void SlotOutputNodeUI::drawCanvas(workflow::WorkflowNode* node) {
  auto* n = static_cast<SlotOutputNode*>(node);

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
  const float node_width = 160.0F * zoom;

  ImGui::Dummy(ImVec2(node_width, 2.0F * zoom));
  ImGui::Text("Slot ID: %d", n->targetSlotId);
  ImGui::Dummy(ImVec2(node_width, 2.0F * zoom));

  PinUI::DrawInput(n->getInputs()[0].get(), "Material", kPinColorMaterial);
  ImGui::Spacing();

  ImNodes::EndNode();

  ImGui::PopStyleColor();
  ImNodes::PopColorStyle();
  ImNodes::PopColorStyle();
}

void SlotOutputNodeUI::drawInspector(workflow::WorkflowNode* node) {
  auto* n = static_cast<SlotOutputNode*>(node);

  ImGui::TextDisabled("Type: Material Slot Output");
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

  int target_id = static_cast<int>(n->targetSlotId);
  if (ImGui::InputInt("Target Slot ID", &target_id)) {
    n->targetSlotId = static_cast<std::uint32_t>(std::max(0, target_id));
    n->markStale();
  }
}

};  // namespace vkit::imgui::windows::ge
