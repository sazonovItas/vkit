#include "vkit/imgui/windows/ge/graph_editor.hpp"

namespace vkit::imgui::windows::ge {

GraphEditorWindow::GraphEditorWindow(const std::string_view name)
    : ImguiWindow(name) {
  ax::NodeEditor::Config config;
  config.SettingsFile = "graph_editor.json";
  editorContext_ = ax::NodeEditor::CreateEditor(&config);
}

GraphEditorWindow::~GraphEditorWindow() {
  if (editorContext_) {
    ax::NodeEditor::DestroyEditor(editorContext_);
    editorContext_ = nullptr;
  }
}

void GraphEditorWindow::onDraw() {
  if (!workflow_) return;

  ed::SetCurrentEditor(editorContext_);

  ed::Begin("Node Canvas");

  for (auto* node : workflow_->getNodes()) {
    auto* wf_node = static_cast<workflow::WorkflowNode*>(node);
    if (auto* ui = registry_.getUI(wf_node)) {
      ui->drawCanvas(wf_node);
    }
  }

  // 2. Add your linking/unlinking logic here...

  ed::Suspend();
  if (ed::ShowBackgroundContextMenu()) {
    ImGui::OpenPopup("GraphCtx");
  }
  if (ImGui::BeginPopup("GraphCtx")) {
    ImGui::EndPopup();
  }
  ed::Resume();

  ed::End();

  selectedNodes_.clear();
  int sel_count = ed::GetSelectedObjectCount();
  if (sel_count > 0) {
    selectedNodes_.resize(sel_count);
    ed::GetSelectedNodes(selectedNodes_.data(), sel_count);
  }

  ed::SetCurrentEditor(nullptr);
}

auto GraphEditorWindow::getSelectedNode() const -> workflow::WorkflowNode* {
  if (selectedNodes_.empty() || !workflow_) return nullptr;

  int raw_id = static_cast<int>(selectedNodes_[0].Get());

  for (auto* node : workflow_->getNodes()) {
    if (node->getId() == raw_id) {
      return static_cast<workflow::WorkflowNode*>(node);
    }
  }
  return nullptr;
}

};  // namespace vkit::imgui::windows::ge
