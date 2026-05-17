#include "vkit/imgui/windows/ge/node/diffuse_specular_ui.hpp"

#include <imgui.h>
#include <imnodes.h>

#include <cstring>

#include "vkit/controller/workflow.hpp"
#include "vkit/imgui/windows/ge/pin_ui.hpp"
#include "vkit/imgui/windows/ge/style.hpp"
#include "vkit/workflow/node/material/diffuse_specular.hpp"

namespace vkit::imgui::windows::ge {

using workflow::node::mat::DiffuseSpecularNode;

static const ImVec4 kPinColorMaterial{0.8F, 0.3F, 0.8F, 1.0F};

auto DiffuseSpecularNodeUI::spawnNode(controller::WorkflowController* controller)
    -> workflow::WorkflowNode* {
  return controller->createDiffuseSpecularNode("Diffuse Specular");
}

void DiffuseSpecularNodeUI::drawCanvas(workflow::WorkflowNode* node) {
  auto* n = static_cast<DiffuseSpecularNode*>(node);

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
  const auto color_tex = style::colors::kPinColorYellow;

  PinUI::DrawInput(inputs[0].get(), "Diffuse", color_tex);
  ImGui::Dummy(ImVec2(0.0F, ImGui::GetStyle().ItemSpacing.y * zoom));
  PinUI::DrawInput(inputs[1].get(), "Specular/Gloss", color_tex);
  ImGui::Dummy(ImVec2(0.0F, ImGui::GetStyle().ItemSpacing.y * zoom));
  PinUI::DrawInput(inputs[2].get(), "Normal Map", color_tex);
  ImGui::Dummy(ImVec2(0.0F, ImGui::GetStyle().ItemSpacing.y * zoom));

  ImGui::Dummy(ImVec2(node_width, 8.0F * zoom));

  PinUI::DrawOutput(n->getOutputs()[0].get(), "Material", kPinColorMaterial,
                    node_width);
  ImGui::Dummy(ImVec2(0.0F, ImGui::GetStyle().ItemSpacing.y * zoom));

  ImNodes::EndNode();

  ImGui::PopStyleColor();
  ImNodes::PopColorStyle();
  ImNodes::PopColorStyle();
}

void DiffuseSpecularNodeUI::drawInspector(workflow::WorkflowNode* node) {
  auto* n = static_cast<DiffuseSpecularNode*>(node);
  bool changed = false;

  ImGui::TextDisabled("Type: Diffuse Specular Material");
  ImGui::Separator();

  char buf[256];
  std::strncpy(buf, n->getName().c_str(), sizeof(buf));
  buf[sizeof(buf) - 1] = '\0';
  if (ImGui::InputText("Node Name", buf, sizeof(buf))) n->setName(buf);

  ImGui::Spacing();

  const char* alpha_modes[] = {"Opaque", "Mask", "Blend"};
  int alpha_idx = static_cast<int>(n->alphaMode);
  if (ImGui::Combo("Alpha Mode", &alpha_idx, alpha_modes, 3)) {
    n->alphaMode = static_cast<material::AlphaMode>(alpha_idx);
    changed = true;
  }

  ImGui::Spacing();
  ImGui::Separator();

  if (ImGui::CollapsingHeader("Properties", ImGuiTreeNodeFlags_DefaultOpen)) {
    if (ImGui::ColorEdit4("Diffuse Color", &n->diffuseFactor[0])) changed = true;
    if (ImGui::ColorEdit3("Specular Color", &n->specularFactor[0])) changed = true;
    if (ImGui::SliderFloat("Glossiness", &n->glossinessFactor, 0.0F, 1.0F))
      changed = true;
  }

  if (changed) n->markStale();
}

};  // namespace vkit::imgui::windows::ge
