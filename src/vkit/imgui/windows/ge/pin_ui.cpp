#include "vkit/imgui/windows/ge/pin_ui.hpp"

#include <imnodes.h>

namespace vkit::imgui::windows::ge {

void PinUI::DrawInput(graph::Pin* pin, const char* label, ImVec4 color,
                      const std::function<void()>& inlineUI) {
  if (!pin) return;

  bool is_connected = !pin->getLinks().empty();
  ImNodesPinShape shape =
      is_connected ? ImNodesPinShape_CircleFilled : ImNodesPinShape_Circle;

  ImNodes::PushColorStyle(ImNodesCol_Pin,
                          ImGui::ColorConvertFloat4ToU32(color));

  ImNodes::BeginInputAttribute(pin->getId(), shape);

  if (label && label[0] != '\0') {
    ImGui::TextUnformatted(label);
  }

  if (inlineUI) {
    if (label && label[0] != '\0') {
      ImGui::SameLine();
    }
    inlineUI();
  }

  ImNodes::EndInputAttribute();

  ImNodes::PopColorStyle();
}

void PinUI::DrawOutput(graph::Pin* pin, const char* label, ImVec4 color,
                       float nodeWidth) {
  if (!pin) return;

  bool is_connected = !pin->getLinks().empty();
  ImNodesPinShape shape =
      is_connected ? ImNodesPinShape_CircleFilled : ImNodesPinShape_Circle;

  ImNodes::PushColorStyle(ImNodesCol_Pin,
                          ImGui::ColorConvertFloat4ToU32(color));

  ImNodes::BeginOutputAttribute(pin->getId(), shape);

  if (label && label[0] != '\0') {
    float text_width = ImGui::CalcTextSize(label).x;
    ImGui::Indent(nodeWidth - text_width);
    ImGui::TextUnformatted(label);
  }

  ImNodes::EndOutputAttribute();

  ImNodes::PopColorStyle();
}

};  // namespace vkit::imgui::windows::ge
