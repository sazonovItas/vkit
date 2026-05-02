#pragma once

#include <imgui.h>
#include <imgui_node_editor.h>
#include <sys/stat.h>

#include <memory>
#include <typeindex>
#include <unordered_map>

#include "vkit/workflow/workflow_node.hpp"

namespace ed = ax::NodeEditor;

namespace vkit::imgui::windows::ge {

class INodeUI {
 public:
  virtual ~INodeUI() = default;

  virtual void drawCanvas(workflow::WorkflowNode* node) = 0;

  virtual void drawInspector(workflow::WorkflowNode* node) = 0;
};

class NodeUIRegistry {
 public:
  template <typename NodeT>
  void registerUI(std::unique_ptr<INodeUI> ui) {
    registry_[typeid(NodeT)] = std::move(ui);
  }

  auto getUI(workflow::WorkflowNode* node) -> INodeUI* {
    if (!node) return nullptr;
    auto it = registry_.find(typeid(*node));
    if (it != registry_.end()) {
      return it->second.get();
    }
    return nullptr;
  }

 private:
  std::unordered_map<std::type_index, std::unique_ptr<INodeUI>> registry_;
};

};  // namespace vkit::imgui::windows::ge
