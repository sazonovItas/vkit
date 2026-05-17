#include "vkit/imgui/windows/ge/node/mix_material_ui.hpp"

#include <imgui.h>
#include <imnodes.h>

#include <cstring>

#include "vkit/controller/workflow.hpp"
#include "vkit/imgui/windows/ge/pin_ui.hpp"
#include "vkit/imgui/windows/ge/style.hpp"
#include "vkit/workflow/node/material/mix_material.hpp"

namespace vkit::imgui::windows::ge {

using workflow::node::mat::MixMaterialNode;

static const ImVec4 kPinColorMaterial{0.8F, 0.3F, 0.8F, 1.0F};

auto MixMaterialNodeUI::spawnNode(controller::WorkflowController* controller)
    -> workflow::WorkflowNode* {
  return controller->createMixMaterialNode("Mix Material");
}

void MixMaterialNodeUI::drawCanvas(workflow::WorkflowNode* node) {
  auto* n = static_cast<MixMaterialNode*>(node);

  const ImU32 header_color =
      ImGui::ColorConvertFloat4ToU32(getStatusColor(n->status()));
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

  const auto& inputs = n->getInputs();

  PinUI::DrawInput(inputs[0].get(), "Material A", kPinColorMaterial);
  ImGui::Dummy(ImVec2(0.0F, ImGui::GetStyle().ItemSpacing.y * zoom));
  PinUI::DrawInput(inputs[1].get(), "Material B", kPinColorMaterial);
  ImGui::Dummy(ImVec2(0.0F, ImGui::GetStyle().ItemSpacing.y * zoom));

  const ImVec4 kPinColorTex{0.8F, 0.7F, 0.2F, 1.0F};
  PinUI::DrawInput(inputs[2].get(), "Factor Map", kPinColorTex);
  ImGui::Dummy(ImVec2(0.0F, ImGui::GetStyle().ItemSpacing.y * zoom));
  const ImVec4 kPinColorOpacity{0.4F, 0.8F, 0.4F, 1.0F};
  PinUI::DrawInput(inputs[3].get(), "Opacity Map", kPinColorOpacity);
  ImGui::Dummy(ImVec2(0.0F, ImGui::GetStyle().ItemSpacing.y * zoom));

  ImNodes::BeginStaticAttribute(n->getId() + 10000);
  ImGui::SetNextItemWidth(node_width * 0.75F);
  if (ImGui::SliderFloat("##factor", &n->factor, 0.0F, 1.0F, "Factor %.2f"))
    n->markStale();
  ImGui::SetNextItemWidth(node_width * 0.75F);
  if (ImGui::SliderFloat("##threshold", &n->threshold, 0.0F, 1.0F, "Threshold %.2f"))
    n->markStale();
  ImGui::SetNextItemWidth(node_width * 0.75F);
  if (ImGui::SliderFloat("##edge", &n->edge, 0.0F, 2.0F, "Edge %.2f"))
    n->markStale();
  ImNodes::EndStaticAttribute();

  ImGui::Dummy(ImVec2(node_width, 6.0F * zoom));

  PinUI::DrawOutput(n->getOutputs()[0].get(), "Material", kPinColorMaterial,
                    node_width);
  ImGui::Dummy(ImVec2(0.0F, ImGui::GetStyle().ItemSpacing.y * zoom));

  ImNodes::EndNode();

  ImGui::PopStyleColor();
  ImNodes::PopColorStyle();
  ImNodes::PopColorStyle();
}

void MixMaterialNodeUI::drawInspector(workflow::WorkflowNode* node) {
  auto* n = static_cast<MixMaterialNode*>(node);
  bool changed = false;

  ImGui::TextDisabled("Type: Mix Material");
  ImGui::Separator();

  char buf[256];
  std::strncpy(buf, n->getName().c_str(), sizeof(buf));
  buf[sizeof(buf) - 1] = '\0';
  if (ImGui::InputText("Node Name", buf, sizeof(buf))) n->setName(buf);

  ImGui::Spacing();
  ImGui::Separator();

  if (ImGui::CollapsingHeader("Properties", ImGuiTreeNodeFlags_DefaultOpen)) {
    const char* alpha_modes[] = {"Opaque", "Mask", "Blend"};
    int alpha_idx = static_cast<int>(n->alphaMode);
    if (ImGui::Combo("Alpha Mode", &alpha_idx, alpha_modes, 3)) {
      n->alphaMode = static_cast<material::AlphaMode>(alpha_idx);
      changed = true;
    }
    ImGui::Spacing();
    if (ImGui::SliderFloat("Alpha Cutoff", &n->alphaCutoff, 0.0F, 1.0F)) changed = true;
    ImGui::Spacing();
    if (ImGui::SliderFloat("Factor", &n->factor, 0.0F, 1.0F)) changed = true;
    ImGui::TextDisabled("0 = Material A,  1 = Material B");
    ImGui::Spacing();
    if (ImGui::SliderFloat("Threshold", &n->threshold, 0.0F, 1.0F)) changed = true;
    ImGui::TextDisabled("Center of the transition zone");
    ImGui::Spacing();
    if (ImGui::SliderFloat("Edge", &n->edge, 0.0F, 2.0F)) changed = true;
    ImGui::TextDisabled("0 = sharp cutoff,  1 = smooth,  2 = near-linear");
  }

  if (changed) n->markStale();
}

}  // namespace vkit::imgui::windows::ge
