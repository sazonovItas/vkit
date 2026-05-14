#include "vkit/imgui/windows/ge/node/pattern_generator_ui.hpp"

#include <imgui.h>
#include <imnodes.h>

#include "vkit/controller/workflow.hpp"
#include "vkit/imgui/windows/ge/pin_ui.hpp"
#include "vkit/imgui/windows/ge/style.hpp"
#include "vkit/workflow/node/procedural/pattern_generator.hpp"

namespace vkit::imgui::windows::ge {

using workflow::node::proc::PatternGeneratorNode;
using workflow::node::proc::PatternParams;
using workflow::node::proc::PatternType;

auto PatternGeneratorNodeUI::spawnNode(
    controller::WorkflowController* controller) -> workflow::WorkflowNode* {
  return controller->createPatternGeneratorNode("Pattern Generator");
}

void PatternGeneratorNodeUI::drawCanvas(workflow::WorkflowNode* node) {
  auto* n = static_cast<PatternGeneratorNode*>(node);

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
  const float node_width = 200.0F * zoom;
  ImGui::Dummy(ImVec2(node_width, 2.0F * zoom));

  const char* type_names[] = {"Brick", "Wood", "Chain"};
  const auto& p = n->getParams();
  ImGui::TextDisabled("%s  %ux%u", type_names[static_cast<uint32_t>(p.type)],
                      p.width, p.height);

  ImGui::Dummy(ImVec2(node_width, 2.0F * zoom));

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

void PatternGeneratorNodeUI::drawInspector(workflow::WorkflowNode* node) {
  auto* n = static_cast<PatternGeneratorNode*>(node);
  PatternParams p = n->getParams();
  bool changed = false;

  ImGui::TextDisabled("Type: Pattern Generator");
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

  {
    const char* items[] = {"Brick", "Wood", "Chain"};
    int cur = static_cast<int>(p.type);
    if (ImGui::Combo("Pattern Type", &cur, items, 3)) {
      p.type = static_cast<PatternType>(cur);
      changed = true;
    }
  }

  ImGui::Spacing();

  {
    int dims[2] = {static_cast<int>(p.width), static_cast<int>(p.height)};
    if (ImGui::InputInt2("Resolution", dims)) {
      p.width = static_cast<uint32_t>(std::max(1, dims[0]));
      p.height = static_cast<uint32_t>(std::max(1, dims[1]));
      changed = true;
    }
  }

  ImGui::Spacing();
  ImGui::Separator();
  ImGui::Spacing();

  if (ImGui::DragFloat("Scale", &p.scale, 0.05F, 0.1F, 100.0F)) changed = true;
  if (ImGui::DragFloat("Thickness", &p.thickness, 0.005F, 0.0F, 1.0F))
    changed = true;
  if (ImGui::DragFloat("Smoothness", &p.smoothness, 0.005F, 0.0F, 1.0F))
    changed = true;

  // Contextual parameters based on type
  if (p.type == PatternType::kBrick) {
    if (ImGui::SliderFloat("Offset Ratio", &p.param1, 0.0F, 1.0F))
      changed = true;
    if (ImGui::SliderFloat("Squash/Stretch", &p.param2, -1.0F, 1.0F))
      changed = true;
  } else if (p.type == PatternType::kWood) {
    if (ImGui::SliderFloat("Ring Density", &p.param1, 0.1F, 20.0F))
      changed = true;
    if (ImGui::SliderFloat("Distortion", &p.param2, 0.0F, 5.0F)) changed = true;
  } else if (p.type == PatternType::kChain) {
    if (ImGui::SliderFloat("Link Width", &p.param1, 0.1F, 1.0F)) changed = true;
    if (ImGui::SliderFloat("Interlock", &p.param2, 0.0F, 1.0F)) changed = true;
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
    ImGui::TextColored(style::colors::kTextExecuting, "Generating...");
  } else if (n->status() == workflow::NodeStatus::kError) {
    ImGui::TextColored(style::colors::kTextError, "Error generating pattern.");
  }
}

}  // namespace vkit::imgui::windows::ge
