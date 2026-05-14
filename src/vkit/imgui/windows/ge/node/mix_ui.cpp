#include "vkit/imgui/windows/ge/node/mix_ui.hpp"

#include <imgui.h>
#include <imnodes.h>

#include "vkit/controller/workflow.hpp"
#include "vkit/imgui/windows/ge/pin_ui.hpp"
#include "vkit/imgui/windows/ge/style.hpp"
#include "vkit/workflow/node/operators/mix.hpp"

namespace vkit::imgui::windows::ge {

using workflow::node::op::MixMode;
using workflow::node::op::MixNode;
using workflow::node::op::MixParams;

auto MixNodeUI::spawnNode(controller::WorkflowController* controller)
    -> workflow::WorkflowNode* {
  return controller->createMixNode("Mix Color");
}

void MixNodeUI::drawCanvas(workflow::WorkflowNode* node) {
  auto* n = static_cast<MixNode*>(node);
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
  ImGui::TextDisabled("Out Size: %dx%d", p.width, p.height);
  ImGui::Dummy(ImVec2(node_width, 2.0F * zoom));

  PinUI::DrawInput(n->getInputs()[0].get(), "Color A",
                   style::colors::kPinColorYellow);
  ImGui::Dummy(ImVec2(0.0f, ImGui::GetStyle().ItemSpacing.y * zoom));
  PinUI::DrawInput(n->getInputs()[1].get(), "Color B",
                   style::colors::kPinColorYellow);
  ImGui::Dummy(ImVec2(0.0f, ImGui::GetStyle().ItemSpacing.y * zoom));
  PinUI::DrawInput(n->getInputs()[2].get(), "Factor (Mask)",
                   style::colors::kPinColorCyan);
  ImGui::Dummy(ImVec2(0.0f, ImGui::GetStyle().ItemSpacing.y * zoom));

  PinUI::DrawOutput(n->getOutputs()[0].get(), "Result (F32)",
                    style::colors::kPinColorCyan, node_width);
  ImGui::Dummy(ImVec2(0.0f, ImGui::GetStyle().ItemSpacing.y * zoom));
  PinUI::DrawOutput(n->getOutputs()[1].get(), "Result (Color)",
                    style::colors::kPinColorYellow, node_width);
  ImGui::Dummy(ImVec2(0.0f, ImGui::GetStyle().ItemSpacing.y * zoom));

  ImNodes::EndNode();
  ImGui::PopStyleColor();
  ImNodes::PopColorStyle();
  ImNodes::PopColorStyle();
}

void MixNodeUI::drawInspector(workflow::WorkflowNode* node) {
  auto* n = static_cast<MixNode*>(node);
  MixParams p = n->getParams();
  bool changed = false;

  ImGui::TextDisabled("Type: Mix Color");
  ImGui::Separator();

  {
    char buf[256];
    std::strncpy(buf, n->getName().c_str(), sizeof(buf));
    buf[sizeof(buf) - 1] = '\0';
    if (ImGui::InputText("Node Name", buf, sizeof(buf))) n->setName(buf);
  }

  ImGui::Spacing();
  ImGui::Separator();

  int dims[2] = {static_cast<int>(p.width), static_cast<int>(p.height)};
  if (ImGui::InputInt2("Output Res", dims)) {
    p.width = static_cast<uint32_t>(std::max(1, dims[0]));
    p.height = static_cast<uint32_t>(std::max(1, dims[1]));
    changed = true;
  }

  ImGui::Spacing();

  const char* mode_names[] = {"Mix", "Multiply", "Add", "Screen"};
  int current_mode = static_cast<int>(p.mode);
  if (ImGui::Combo("Blend Mode", &current_mode, mode_names, 4)) {
    p.mode = static_cast<MixMode>(current_mode);
    changed = true;
  }

  if (ImGui::SliderFloat("Factor", &p.factor, 0.0F, 1.0F)) {
    changed = true;
  }

  if (changed) n->setParams(p);

  if (n->status() == workflow::NodeStatus::kReady &&
      n->outputColorId.has_value() && textureManager_) {
    auto tex = textureManager_->get(n->outputColorId.value());
    if (tex && tex->getImguiId().has_value()) {
      ImGui::Spacing();
      ImGui::Separator();
      ImGui::Spacing();
      float panel_width = ImGui::GetContentRegionAvail().x;
      ImGui::Image(*tex->getImguiId(), ImVec2(panel_width, panel_width));
    }
  }
}
};  // namespace vkit::imgui::windows::ge
