#include "vkit/imgui/windows/ge/node/heightmap_ui.hpp"

#include <imgui.h>
#include <imnodes.h>

#include "vkit/controller/workflow.hpp"
#include "vkit/imgui/windows/ge/pin_ui.hpp"
#include "vkit/imgui/windows/ge/style.hpp"
#include "vkit/workflow/node/operators/heightmap.hpp"

namespace vkit::imgui::windows::ge {

using workflow::node::op::HeightMapNode;
using workflow::node::op::HeightMapParams;

auto HeightMapNodeUI::spawnNode(controller::WorkflowController* controller)
    -> workflow::WorkflowNode* {
  return controller->createHeightMapNode("Height Map");
}

void HeightMapNodeUI::drawCanvas(workflow::WorkflowNode* node) {
  auto* n = static_cast<HeightMapNode*>(node);

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
  ImGui::TextDisabled(p.invert ? "Inverted" : "Normal");

  ImGui::Dummy(ImVec2(node_width, 2.0F * zoom));

  PinUI::DrawInput(n->getInputs()[0].get(), "Color",
                   style::colors::kPinColorYellow);
  ImGui::Dummy(ImVec2(0.0f, ImGui::GetStyle().ItemSpacing.y * zoom));

  PinUI::DrawOutput(n->getOutputs()[0].get(), "Image",
                    style::colors::kPinColorCyan, node_width);
  ImGui::Dummy(ImVec2(0.0f, ImGui::GetStyle().ItemSpacing.y * zoom));
  PinUI::DrawOutput(n->getOutputs()[1].get(), "Color",
                    style::colors::kPinColorYellow, node_width);
  ImGui::Dummy(ImVec2(0.0f, ImGui::GetStyle().ItemSpacing.y * zoom));

  ImNodes::EndNode();

  ImGui::PopStyleColor();
  ImNodes::PopColorStyle();
  ImNodes::PopColorStyle();
}

void HeightMapNodeUI::drawInspector(workflow::WorkflowNode* node) {
  auto* n = static_cast<HeightMapNode*>(node);
  HeightMapParams p = n->getParams();
  bool changed = false;

  ImGui::TextDisabled("Type: Height Map");
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

  if (ImGui::DragFloat("Contrast", &p.contrast, 0.02F, 0.0F, 5.0F))
    changed = true;
  if (ImGui::DragFloat("Brightness", &p.brightness, 0.01F, -1.0F, 1.0F))
    changed = true;

  bool invert_bool = (p.invert != 0);
  if (ImGui::Checkbox("Invert", &invert_bool)) {
    p.invert = invert_bool ? 1 : 0;
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
