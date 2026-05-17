#pragma once

#include <imnodes.h>

#include <filesystem>
#include <string_view>

#include "vkit/controller/workflow.hpp"
#include "vkit/imgui/imgui_window.hpp"
#include "vkit/imgui/windows/ge/node/node_ui.hpp"

namespace vkit::imgui::windows::ge {

class GraphEditorWindow : public ImguiWindow {
 public:
  explicit GraphEditorWindow(std::string_view name);
  ~GraphEditorWindow() override;

  void onDraw() override;

  void setController(controller::WorkflowController* controller) {
    controller_ = controller;
  }

  controller::WorkflowController* getController() const { return controller_; }

  NodeUIRegistry& getRegistry() { return registry_; }

 private:
  controller::WorkflowController* controller_{nullptr};
  NodeUIRegistry registry_;

  bool pendingExport_{false};
  bool pendingImport_{false};
  std::filesystem::path pendingExportPath_;
  std::filesystem::path pendingImportPath_;
};

};  // namespace vkit::imgui::windows::ge
