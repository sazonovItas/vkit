#pragma once

#include <string_view>

#include "vkit/core/events/compute_output.hpp"
#include "vkit/workflow/compute_output_node.hpp"

namespace vkit::workflow::node::op {

struct ChannelAdjustParams {
  float gain[4]{1.0F, 1.0F, 1.0F, 1.0F};
  float bias[4]{0.0F, 0.0F, 0.0F, 0.0F};
  bool  invert[4]{false, false, false, false};
};

class ChannelAdjustNode : public workflow::ComputeOutputNode {
 public:
  ChannelAdjustNode(std::string_view name, texture::TextureManager& mgr,
                    core::events::ComputeOutputBus& bus,
                    core::events::ComputeOutputResultBus& resultBus,
                    core::events::ComputeHandles handles);

  void execute() override;
  void setParams(ChannelAdjustParams p);
  [[nodiscard]] auto getParams() const -> const ChannelAdjustParams& {
    return params_;
  }

 private:
  ChannelAdjustParams params_;
  graph::Pin* inImage_{nullptr};
};

}  // namespace vkit::workflow::node::op
