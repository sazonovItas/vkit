#pragma once

#include <imgui.h>
#include <imnodes.h>

#include "vkit/imgui/imgui_window.hpp"
#include "vkit/imgui/windows/ge/graph_editor.hpp"
#include "vkit/workflow/workflow.hpp"

namespace vkit::imgui::windows::ge {

class GraphNodeInspectorWindow : public ImguiWindow {
 public:
  explicit GraphNodeInspectorWindow(const std::string_view name,
                                    GraphEditorWindow* graphEditor)
      : ImguiWindow(name), graphEditor_(graphEditor) {}

  void onDraw() override {
    if (!graphEditor_ || !graphEditor_->getController()) return;

    auto* workflow = graphEditor_->getController()->getWorkflow();
    if (!workflow) return;

    ImGui::BeginChild("InspectorScrollRegion", ImVec2(0, 0), 0,
                      ImGuiWindowFlags_AlwaysVerticalScrollbar);

    int selected_count = ImNodes::NumSelectedNodes();

    if (selected_count == 0) {
      ImGui::TextDisabled("Select a node to inspect its properties.");
      ImGui::EndChild();
      return;
    }

    if (selected_count > 1) {
      ImGui::TextDisabled("%d nodes selected. Bulk editing not supported.",
                          selected_count);
      ImGui::EndChild();
      return;
    }

    int selected_id = -1;
    ImNodes::GetSelectedNodes(&selected_id);

    workflow::WorkflowNode* selected_node = nullptr;
    for (auto* base_node : workflow->getNodes()) {
      if (base_node->getId() == selected_id) {
        selected_node = static_cast<workflow::WorkflowNode*>(base_node);
        break;
      }
    }

    if (!selected_node) {
      ImGui::TextDisabled("Selected item could not be found in the workflow.");
      ImGui::EndChild();
      return;
    }

    if (auto* ui = graphEditor_->getRegistry().getUI(selected_node)) {
      ui->drawInspector(selected_node);
    } else {
      ImGui::TextDisabled("No Inspector UI registered for this node type.");
    }

    ImGui::EndChild();
  }

 private:
  GraphEditorWindow* graphEditor_;
};

};  // namespace vkit::imgui::windows::ge
