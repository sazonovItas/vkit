#include "vkit/imgui/windows/ge/node/channel_remap_ui.hpp"

#include <imgui.h>
#include <imnodes.h>

#include <cstring>

#include "vkit/controller/workflow.hpp"
#include "vkit/imgui/windows/ge/pin_ui.hpp"
#include "vkit/imgui/windows/ge/style.hpp"
#include "vkit/workflow/node/operators/channel_remap.hpp"

namespace vkit::imgui::windows::ge {

using workflow::node::op::ChannelRemapNode;
using workflow::node::op::ChannelRemapParams;

static const char* kChannelNames[] = {"R", "G", "B", "A", "0", "1"};

auto ChannelRemapNodeUI::spawnNode(controller::WorkflowController* controller)
    -> workflow::WorkflowNode* {
  return controller->createChannelRemapNode("Channel Remap");
}

void ChannelRemapNodeUI::drawCanvas(workflow::WorkflowNode* node) {
  auto* n = static_cast<ChannelRemapNode*>(node);

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
  ImGui::TextDisabled("R←%s  G←%s  B←%s  A←%s",
                      kChannelNames[p.outR], kChannelNames[p.outG],
                      kChannelNames[p.outB], kChannelNames[p.outA]);

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

void ChannelRemapNodeUI::drawInspector(workflow::WorkflowNode* node) {
  auto* n = static_cast<ChannelRemapNode*>(node);
  ChannelRemapParams p = n->getParams();
  bool changed = false;

  ImGui::TextDisabled("Type: Channel Remap");
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

  auto channel_combo = [&](const char* label, uint32_t& ch) {
    int cur = static_cast<int>(ch);
    if (ImGui::Combo(label, &cur, kChannelNames, 6)) {
      ch = static_cast<uint32_t>(cur);
      changed = true;
    }
  };

  channel_combo("Out R  ←", p.outR);
  channel_combo("Out G  ←", p.outG);
  channel_combo("Out B  ←", p.outB);
  channel_combo("Out A  ←", p.outA);

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
