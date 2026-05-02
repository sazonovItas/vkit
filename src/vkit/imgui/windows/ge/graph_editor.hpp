#pragma once

#include <imnodes.h>

#include <string_view>

#include "vkit/controller/workflow.hpp"
#include "vkit/imgui/imgui_window.hpp"
#include "vkit/imgui/windows/ge/node_ui.hpp"

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
};

};  // namespace vkit::imgui::windows::ge
