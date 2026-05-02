#pragma once
#include "vkit/imgui/windows/ge/node_ui.hpp"
#include "vkit/texture/manager.hpp"

namespace vkit::imgui::windows::ge {

class TextureLoadNodeUI : public INodeUI {
 public:
  explicit TextureLoadNodeUI(texture::TextureManager* manager)
      : textureManager_(manager) {}

  void drawCanvas(workflow::WorkflowNode* node) override;
  void drawInspector(workflow::WorkflowNode* node) override;

 private:
  texture::TextureManager* textureManager_{nullptr};
};

};  // namespace vkit::imgui::windows::ge
