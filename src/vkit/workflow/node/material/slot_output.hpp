#pragma once

#include <string_view>

#include "vkit/material/manager.hpp"
#include "vkit/workflow/workflow_node.hpp"

namespace vkit::workflow::node::mat {

class SlotOutputNode : public WorkflowNode {
 public:
  SlotOutputNode(std::string_view name, material::MaterialManager& matManager);
  ~SlotOutputNode() override = default;

  void execute() override;

  std::uint32_t targetSlotId{0};

 private:
  material::MaterialManager& matManager_;
  graph::Pin* inMaterial_{nullptr};
};

};  // namespace vkit::workflow::node::mat
