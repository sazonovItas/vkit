#include "vkit/imgui/windows/ge/node/fractal_generator_ui.hpp"

#include <imgui.h>
#include <imnodes.h>

#include "vkit/controller/workflow.hpp"
#include "vkit/imgui/windows/ge/pin_ui.hpp"
#include "vkit/imgui/windows/ge/style.hpp"
#include "vkit/workflow/node/procedural/fractal_generator.hpp"

namespace vkit::imgui::windows::ge {

using workflow::node::proc::FractalGeneratorNode;
using workflow::node::proc::FractalParams;
using workflow::node::proc::FractalType;

auto FractalGeneratorNodeUI::spawnNode(controller::WorkflowController* controller)
    -> workflow::WorkflowNode* {
  return controller->createFractalGeneratorNode("Fractal Generator");
}

void FractalGeneratorNodeUI::drawCanvas(workflow::WorkflowNode* node) {
  auto* n = static_cast<FractalGeneratorNode*>(node);

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

  const char* type_names[] = {"Mandelbrot", "Julia", "Burning Ship"};
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

void FractalGeneratorNodeUI::drawInspector(workflow::WorkflowNode* node) {
  auto* n = static_cast<FractalGeneratorNode*>(node);
  FractalParams p = n->getParams();
  bool changed = false;

  ImGui::TextDisabled("Type: Fractal Generator");
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
    const char* items[] = {"Mandelbrot", "Julia", "Burning Ship"};
    int cur = static_cast<int>(p.type);
    if (ImGui::Combo("Fractal Type", &cur, items, 3)) {
      p.type = static_cast<FractalType>(cur);
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

  if (ImGui::SliderInt("Max Iterations", &p.maxIterations, 16, 1024))
    changed = true;

  ImGui::Spacing();
  ImGui::Separator();
  ImGui::TextDisabled("View");

  if (ImGui::DragFloat("Center X", &p.centerX, 0.005F)) changed = true;
  if (ImGui::DragFloat("Center Y", &p.centerY, 0.005F)) changed = true;
  if (ImGui::DragFloat("Zoom", &p.zoom, 0.05F, 0.01F, 1e6F, "%.3f",
                        ImGuiSliderFlags_Logarithmic))
    changed = true;

  if (p.type == FractalType::kJulia) {
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::TextDisabled("Julia c");
    if (ImGui::DragFloat("Re(c)", &p.juliaRe, 0.005F, -2.0F, 2.0F))
      changed = true;
    if (ImGui::DragFloat("Im(c)", &p.juliaIm, 0.005F, -2.0F, 2.0F))
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
    ImGui::TextColored(style::colors::kTextExecuting, "Generating...");
  } else if (n->status() == workflow::NodeStatus::kError) {
    ImGui::TextColored(style::colors::kTextError, "Error generating fractal.");
  }
}

}  // namespace vkit::imgui::windows::ge
