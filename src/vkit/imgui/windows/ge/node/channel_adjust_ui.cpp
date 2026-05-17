#include "vkit/imgui/windows/ge/node/channel_adjust_ui.hpp"

#include <imgui.h>
#include <imnodes.h>

#include <cstring>

#include "vkit/controller/workflow.hpp"
#include "vkit/imgui/windows/ge/pin_ui.hpp"
#include "vkit/imgui/windows/ge/style.hpp"
#include "vkit/workflow/node/operators/channel_adjust.hpp"

namespace vkit::imgui::windows::ge {

using workflow::node::op::ChannelAdjustNode;
using workflow::node::op::ChannelAdjustParams;

static const char* kChLabels[4] = {"R", "G", "B", "A"};

auto ChannelAdjustNodeUI::spawnNode(controller::WorkflowController* controller)
    -> workflow::WorkflowNode* {
  return controller->createChannelAdjustNode("Channel Adjust");
}

void ChannelAdjustNodeUI::drawCanvas(workflow::WorkflowNode* node) {
  auto* n = static_cast<ChannelAdjustNode*>(node);

  ImNodes::PushColorStyle(ImNodesCol_TitleBar,
      ImGui::ColorConvertFloat4ToU32(getStatusColor(n->status())));
  ImNodes::PushColorStyle(ImNodesCol_NodeBackground,
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
  ImGui::TextDisabled("G: %.2f %.2f %.2f %.2f",
      p.gain[0], p.gain[1], p.gain[2], p.gain[3]);
  ImGui::TextDisabled("B: %.2f %.2f %.2f %.2f",
      p.bias[0], p.bias[1], p.bias[2], p.bias[3]);

  ImGui::Dummy(ImVec2(node_width, 2.0F * zoom));

  PinUI::DrawInput(n->getInputs()[0].get(), "Image", style::colors::kPinColorYellow);
  ImGui::Dummy(ImVec2(0.0F, ImGui::GetStyle().ItemSpacing.y * zoom));

  PinUI::DrawOutput(n->getOutputs()[0].get(), "Image", style::colors::kPinColorCyan, node_width);
  ImGui::Dummy(ImVec2(0.0F, ImGui::GetStyle().ItemSpacing.y * zoom));
  PinUI::DrawOutput(n->getOutputs()[1].get(), "Color", style::colors::kPinColorYellow, node_width);
  ImGui::Dummy(ImVec2(0.0F, ImGui::GetStyle().ItemSpacing.y * zoom));

  ImNodes::EndNode();
  ImGui::PopStyleColor();
  ImNodes::PopColorStyle();
  ImNodes::PopColorStyle();
}

void ChannelAdjustNodeUI::drawInspector(workflow::WorkflowNode* node) {
  auto* n = static_cast<ChannelAdjustNode*>(node);
  ChannelAdjustParams p = n->getParams();
  bool changed = false;

  ImGui::TextDisabled("Type: Channel Adjust");
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

  static const ImVec4 kChColors[4] = {
      {1.0F, 0.3F, 0.3F, 1.0F},
      {0.3F, 1.0F, 0.3F, 1.0F},
      {0.3F, 0.6F, 1.0F, 1.0F},
      {0.8F, 0.8F, 0.8F, 1.0F},
  };

  for (int i = 0; i < 4; ++i) {
    ImGui::PushStyleColor(ImGuiCol_Text, kChColors[i]);
    ImGui::TextUnformatted(kChLabels[i]);
    ImGui::PopStyleColor();
    ImGui::SameLine();

    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.45F);
    char gain_id[16]; std::snprintf(gain_id, sizeof(gain_id), "##gain%d", i);
    if (ImGui::DragFloat(gain_id, &p.gain[i], 0.01F, 0.0F, 8.0F, "G: %.2f"))
      changed = true;
    ImGui::SameLine();
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.8F);
    char bias_id[16]; std::snprintf(bias_id, sizeof(bias_id), "##bias%d", i);
    if (ImGui::DragFloat(bias_id, &p.bias[i], 0.01F, -1.0F, 1.0F, "B: %.2f"))
      changed = true;
    ImGui::SameLine();
    char inv_id[16]; std::snprintf(inv_id, sizeof(inv_id), "##inv%d", i);
    if (ImGui::Checkbox(inv_id, &p.invert[i])) changed = true;
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Invert");
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
        float w = ImGui::GetContentRegionAvail().x;
        ImGui::Image(*tex->getImguiId(), ImVec2(w, w));
      }
    }
  } else if (n->status() == workflow::NodeStatus::kExecuting) {
    ImGui::TextColored(style::colors::kTextExecuting, "Processing...");
  } else if (n->status() == workflow::NodeStatus::kError) {
    ImGui::TextColored(style::colors::kTextError, "Error processing image.");
  }
}

};  // namespace vkit::imgui::windows::ge
