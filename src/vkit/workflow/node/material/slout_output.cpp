#include "vkit/graph/link.hpp"
#include "vkit/workflow/node/material/material_link.hpp"
#include "vkit/workflow/node/material/slot_output.hpp"
#include "vkit/workflow/pin_type.hpp"

namespace vkit::workflow::node::mat {

SlotOutputNode::SlotOutputNode(std::string_view name,
                               material::MaterialManager& matManager)
    : WorkflowNode(name), matManager_(matManager) {
  inMaterial_ = addInputPin(pinKeyType(PinType::kMaterial), "Material");
}

void SlotOutputNode::execute() {
  auto slot = matManager_.getSlot(targetSlotId);
  if (!slot) {
    while (!matManager_.getSlot(targetSlotId)) {
      matManager_.addSlot(std::make_shared<material::Slot>());
    }
    slot = matManager_.getSlot(targetSlotId);

    if (!slot) {
      setStatus(NodeStatus::kError);
      return;
    }
  }

  const MaterialLink* link_data = nullptr;
  bool upstream_waiting = false;

  if (inMaterial_->hasData()) {
    link_data = inMaterial_->getData<MaterialLink>();
  } else {
    auto links = inMaterial_->getLinks();
    if (!links.empty()) {
      auto* source_pin = links.front()->getSrc();
      auto* source_node =
          static_cast<workflow::WorkflowNode*>(source_pin->getOwnerNode());

      if (source_node->status() != NodeStatus::kReady) {
        upstream_waiting = true;
      }

      link_data = source_pin->getData<MaterialLink>();
    }
  }

  if (link_data) {
    slot->setMaterialType(link_data->type);
    slot->setMaterialId(link_data->managerId);
  } else {
    slot->setMaterialType(material::Type::kNone);
    slot->setMaterialId(0);
  }

  if (upstream_waiting) {
    setStatus(NodeStatus::kStale);
  } else {
    setStatus(NodeStatus::kReady);
  }
}

};  // namespace vkit::workflow::node::mat
