#pragma once

#include <imgui.h>

#include "vkit/imgui/imgui_window.hpp"
#include "vkit/imgui/windows/ge/graph_editor.hpp"

namespace vkit::imgui::windows::ge {

class GraphNodeInspectorWindow : public ImguiWindow {
 public:
  explicit GraphNodeInspectorWindow(const std::string_view name,
                                    GraphEditorWindow* graphEditor)
      : ImguiWindow(name), graphEditor_(graphEditor) {}

  void onDraw() override {
    if (!graphEditor_) return;

    auto* selected_node = graphEditor_->getSelectedNode();

    if (!selected_node) {
      ImGui::TextDisabled("Select a node to inspect its properties.");
      return;
    }

    if (auto* ui = graphEditor_->getRegistry().getUI(selected_node)) {
      ui->drawInspector(selected_node);
    } else {
      ImGui::TextDisabled("No Inspector UI registered for this node type.");
    }
  }

 private:
  ge::GraphEditorWindow* graphEditor_;
};

};  // namespace vkit::imgui::windows::ge
