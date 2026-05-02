#pragma once

#include <imgui.h>
#include <imnodes.h>

#include <map>
#include <memory>
#include <string>
#include <typeindex>
#include <unordered_map>
#include <vector>

#include "vkit/controller/workflow.hpp"
#include "vkit/workflow/workflow_node.hpp"

namespace vkit::imgui::windows::ge {

class INodeUI {
 public:
  virtual ~INodeUI() = default;

  virtual auto getName() const -> const char* = 0;
  virtual auto getCategory() const -> const char* = 0;
  virtual auto spawnNode(controller::WorkflowController* controller)
      -> workflow::WorkflowNode* = 0;

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

  void drawCreationMenu(controller::WorkflowController* controller,
                        ImVec2 spawn_pos) {
    std::map<std::string, std::vector<INodeUI*>> categories;
    for (const auto& [type, ui] : registry_) {
      categories[ui->getCategory()].push_back(ui.get());
    }

    for (const auto& [category, uis] : categories) {
      if (ImGui::BeginMenu(category.c_str())) {
        for (auto* ui : uis) {
          if (ImGui::MenuItem(ui->getName())) {
            if (auto* new_node = ui->spawnNode(controller)) {
              ImNodes::SetNodeScreenSpacePos(new_node->getId(), spawn_pos);
            }
          }
        }
        ImGui::EndMenu();
      }
    }
  }

 private:
  std::unordered_map<std::type_index, std::unique_ptr<INodeUI>> registry_;
};

};  // namespace vkit::imgui::windows::ge
