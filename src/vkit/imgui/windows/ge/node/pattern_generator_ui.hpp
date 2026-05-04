#pragma once

#include "vkit/imgui/windows/ge/node/node_ui.hpp"
#include "vkit/texture/manager.hpp"

namespace vkit::imgui::windows::ge {

class PatternGeneratorNodeUI : public INodeUI {
 public:
  explicit PatternGeneratorNodeUI(texture::TextureManager* textureManager)
      : textureManager_{textureManager} {}

  auto getName() const -> const char* override { return "Pattern Generator"; }
  auto getCategory() const -> const char* override { return "Procedural"; }

  auto spawnNode(controller::WorkflowController* controller)
      -> workflow::WorkflowNode* override;

  void drawCanvas(workflow::WorkflowNode* node) override;
  void drawInspector(workflow::WorkflowNode* node) override;

 private:
  texture::TextureManager* textureManager_{nullptr};
};

}  // namespace vkit::imgui::windows::ge
