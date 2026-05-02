#pragma once

#include <imgui.h>
#include <imgui_node_editor.h>

#include <memory>
#include <vector>

#include "vkit/imgui/imgui_window.hpp"
#include "vkit/imgui/windows/ge/node_ui.hpp"
#include "vkit/workflow/workflow.hpp"

namespace vkit::imgui::windows::ge {

class GraphEditorWindow : public ImguiWindow {
 public:
  explicit GraphEditorWindow(std::string_view name);
  ~GraphEditorWindow() override;

  void onDraw() override;

  void setWorkflow(workflow::Workflow* wf) { workflow_ = wf; }
  auto getRegistry() -> NodeUIRegistry& { return registry_; }

  [[nodiscard]] auto getSelectedNode() const -> workflow::WorkflowNode*;

 private:
  workflow::Workflow* workflow_{nullptr};
  ax::NodeEditor::EditorContext* editorContext_{nullptr};

  NodeUIRegistry registry_;
  std::vector<ax::NodeEditor::NodeId> selectedNodes_;
};

};  // namespace vkit::imgui::windows::ge
