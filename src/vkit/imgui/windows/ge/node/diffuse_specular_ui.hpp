#pragma once

#include "vkit/imgui/windows/ge/node/node_ui.hpp"

namespace vkit::imgui::windows::ge {

class DiffuseSpecularNodeUI : public INodeUI {
 public:
  auto getName() const -> const char* override { return "Diffuse Specular"; }
  auto getCategory() const -> const char* override { return "Material"; }

  auto spawnNode(controller::WorkflowController* controller)
      -> workflow::WorkflowNode* override;

  void drawCanvas(workflow::WorkflowNode* node) override;
  void drawInspector(workflow::WorkflowNode* node) override;
};

};  // namespace vkit::imgui::windows::ge
