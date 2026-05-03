#pragma once

#include "vkit/imgui/windows/ge/node/node_ui.hpp"
#include "vkit/texture/manager.hpp"

namespace vkit::imgui::windows::ge {

class MixNodeUI : public INodeUI {
 public:
  explicit MixNodeUI(texture::TextureManager* textureManager)
      : textureManager_{textureManager} {}

  auto getName() const -> const char* override { return "Mix"; }
  auto getCategory() const -> const char* override { return "Operators"; }

  auto spawnNode(controller::WorkflowController* controller)
      -> workflow::WorkflowNode* override;

  void drawCanvas(workflow::WorkflowNode* node) override;
  void drawInspector(workflow::WorkflowNode* node) override;

 private:
  texture::TextureManager* textureManager_{nullptr};
};

};  // namespace vkit::imgui::windows::ge
