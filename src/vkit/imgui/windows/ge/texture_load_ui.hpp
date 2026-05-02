#pragma once

#include "vkit/imgui/windows/ge/node_ui.hpp"
#include "vkit/texture/manager.hpp"

namespace vkit::imgui::windows::ge {

class TextureLoadNodeUI : public INodeUI {
 public:
  explicit TextureLoadNodeUI(texture::TextureManager* textureManager)
      : textureManager_(textureManager) {}

  void drawCanvas(workflow::WorkflowNode* node) override;
  void drawInspector(workflow::WorkflowNode* node) override;

  auto getName() const -> const char* override { return "Texture Load"; }
  auto getCategory() const -> const char* override { return "I/O Nodes"; }

  auto spawnNode(controller::WorkflowController* controller)
      -> workflow::WorkflowNode* override {
    return controller->createTextureLoadNode();
  }

 private:
  texture::TextureManager* textureManager_;
};

};  // namespace vkit::imgui::windows::ge
