#include "vkit/imgui/windows/ge/node/principled_bsdf_ui.hpp"

#include <imgui.h>
#include <imnodes.h>

#include <cstring>

#include "vkit/controller/workflow.hpp"
#include "vkit/imgui/windows/ge/pin_ui.hpp"
#include "vkit/imgui/windows/ge/style.hpp"
#include "vkit/workflow/node/material/principled_bsdf.hpp"

namespace vkit::imgui::windows::ge {

using workflow::node::mat::PrincipledBSDFNode;

const ImVec4 kPinColorMaterial = ImVec4(0.8F, 0.3F, 0.8F, 1.0F);

auto PrincipledBSDFNodeUI::spawnNode(controller::WorkflowController* controller)
    -> workflow::WorkflowNode* {
  return controller->createPrincipledBSDFNode("Principled BSDF");
}

void PrincipledBSDFNodeUI::drawCanvas(workflow::WorkflowNode* node) {
  auto* n = static_cast<workflow::node::mat::PrincipledBSDFNode*>(node);

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

  const auto& inputs = n->getInputs();
  auto color_tex = style::colors::kPinColorYellow;

  PinUI::DrawInput(inputs[0].get(), "Base Color", color_tex);
  ImGui::Dummy(ImVec2(0.0f, ImGui::GetStyle().ItemSpacing.y * zoom));
  PinUI::DrawInput(inputs[1].get(), "Normal Map", color_tex);
  ImGui::Dummy(ImVec2(0.0f, ImGui::GetStyle().ItemSpacing.y * zoom));
  PinUI::DrawInput(inputs[2].get(), "Met/Rough", color_tex);
  ImGui::Dummy(ImVec2(0.0f, ImGui::GetStyle().ItemSpacing.y * zoom));
  PinUI::DrawInput(inputs[3].get(), "Emissive", color_tex);
  ImGui::Dummy(ImVec2(0.0f, ImGui::GetStyle().ItemSpacing.y * zoom));
  PinUI::DrawInput(inputs[4].get(), "Ambient Occlusion", color_tex);
  ImGui::Dummy(ImVec2(0.0f, ImGui::GetStyle().ItemSpacing.y * zoom));

  PinUI::DrawInput(inputs[5].get(), "Specular", color_tex);
  ImGui::Dummy(ImVec2(0.0f, ImGui::GetStyle().ItemSpacing.y * zoom));
  PinUI::DrawInput(inputs[6].get(), "Specular Color", color_tex);
  ImGui::Dummy(ImVec2(0.0f, ImGui::GetStyle().ItemSpacing.y * zoom));

  PinUI::DrawInput(inputs[7].get(), "Thickness", color_tex);
  ImGui::Dummy(ImVec2(0.0f, ImGui::GetStyle().ItemSpacing.y * zoom));

  PinUI::DrawInput(inputs[8].get(), "Clearcoat", color_tex);
  ImGui::Dummy(ImVec2(0.0f, ImGui::GetStyle().ItemSpacing.y * zoom));
  PinUI::DrawInput(inputs[9].get(), "Clearcoat Normal", color_tex);
  ImGui::Dummy(ImVec2(0.0f, ImGui::GetStyle().ItemSpacing.y * zoom));

  PinUI::DrawInput(inputs[10].get(), "Sheen Color", color_tex);
  ImGui::Dummy(ImVec2(0.0f, ImGui::GetStyle().ItemSpacing.y * zoom));
  PinUI::DrawInput(inputs[11].get(), "Sheen Roughness", color_tex);
  ImGui::Dummy(ImVec2(0.0f, ImGui::GetStyle().ItemSpacing.y * zoom));

  PinUI::DrawInput(inputs[12].get(), "Anisotropy", color_tex);
  ImGui::Dummy(ImVec2(0.0f, ImGui::GetStyle().ItemSpacing.y * zoom));

  PinUI::DrawInput(inputs[13].get(), "Iridescence", color_tex);
  ImGui::Dummy(ImVec2(0.0f, ImGui::GetStyle().ItemSpacing.y * zoom));
  PinUI::DrawInput(inputs[14].get(), "Irid. Thickness", color_tex);
  ImGui::Dummy(ImVec2(0.0f, ImGui::GetStyle().ItemSpacing.y * zoom));

  ImGui::Dummy(ImVec2(node_width, 8.0F * zoom));

  PinUI::DrawOutput(n->getOutputs()[0].get(), "Material", kPinColorMaterial,
                    node_width);
  ImGui::Dummy(ImVec2(0.0f, ImGui::GetStyle().ItemSpacing.y * zoom));

  ImNodes::EndNode();

  ImGui::PopStyleColor();
  ImNodes::PopColorStyle();
  ImNodes::PopColorStyle();
}

void PrincipledBSDFNodeUI::drawInspector(workflow::WorkflowNode* node) {
  auto* n = static_cast<workflow::node::mat::PrincipledBSDFNode*>(node);
  bool changed = false;

  ImGui::TextDisabled("Type: Principled BSDF Material");
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

  if (ImGui::CollapsingHeader("Base Properties",
                              ImGuiTreeNodeFlags_DefaultOpen)) {
    if (ImGui::ColorEdit4("Base Color", &n->baseColorFactor[0])) changed = true;
    if (ImGui::SliderFloat("Metallic", &n->metallicFactor, 0.0F, 1.0F))
      changed = true;
    if (ImGui::SliderFloat("Roughness", &n->roughnessFactor, 0.0F, 1.0F))
      changed = true;
    if (ImGui::SliderFloat("Occlusion", &n->occlusionStrength, 0.0F, 1.0F))
      changed = true;
    if (ImGui::SliderFloat("IOR", &n->ior, 1.0F, 3.0F)) changed = true;

    if (ImGui::ColorEdit3("Emissive", &n->emissiveFactor[0],
                          ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_Float))
      changed = true;
  }

  if (ImGui::CollapsingHeader("Specular & Transmission")) {
    if (ImGui::ColorEdit3("Specular Color", &n->specularColorFactor[0]))
      changed = true;
    if (ImGui::SliderFloat("Specular Factor", &n->specularFactor, 0.0F, 1.0F))
      changed = true;

    ImGui::Separator();
    if (ImGui::SliderFloat("Transmission", &n->transmissionFactor, 0.0F, 1.0F))
      changed = true;
    if (ImGui::DragFloat("Thickness", &n->thicknessFactor, 0.01F, 0.0F, 10.0F))
      changed = true;
    if (ImGui::DragFloat("Atten. Dist", &n->attenuationDistance, 0.1F, 0.0F,
                         100.0F))
      changed = true;
    if (ImGui::ColorEdit3("Atten. Color", &n->attenuationColor[0]))
      changed = true;
  }

  if (ImGui::CollapsingHeader("Clearcoat & Sheen")) {
    if (ImGui::SliderFloat("Clearcoat", &n->clearcoatFactor, 0.0F, 1.0F))
      changed = true;
    if (ImGui::SliderFloat("CC Roughness", &n->clearcoatRoughnessFactor, 0.0F,
                           1.0F))
      changed = true;

    ImGui::Separator();
    if (ImGui::ColorEdit3("Sheen Color", &n->sheenColorFactor[0]))
      changed = true;
    if (ImGui::SliderFloat("Sheen Roughness", &n->sheenRoughnessFactor, 0.0F,
                           1.0F))
      changed = true;
  }

  if (ImGui::CollapsingHeader("Anisotropy & Iridescence")) {
    if (ImGui::SliderFloat("Anisotropy", &n->anisotropyStrength, 0.0F, 1.0F))
      changed = true;
    if (ImGui::SliderFloat2("Aniso. Rotation", &n->anisotropyRotation[0], -1.0F,
                            1.0F))
      changed = true;

    ImGui::Separator();
    if (ImGui::SliderFloat("Iridescence", &n->iridescenceFactor, 0.0F, 1.0F))
      changed = true;
    if (ImGui::SliderFloat("Irid. IOR", &n->iridescenceIor, 1.0F, 3.0F))
      changed = true;
    if (ImGui::DragFloat("Irid. Min Thick", &n->iridescenceThicknessMin, 1.0F,
                         0.0F, 1000.0F))
      changed = true;
    if (ImGui::DragFloat("Irid. Max Thick", &n->iridescenceThicknessMax, 1.0F,
                         0.0F, 1000.0F))
      changed = true;
  }

  if (changed) {
    n->markStale();
  }
}

};  // namespace vkit::imgui::windows::ge
