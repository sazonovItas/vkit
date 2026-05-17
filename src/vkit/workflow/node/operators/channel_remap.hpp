#pragma once

#include <string_view>

#include "vkit/core/events/compute_output.hpp"
#include "vkit/workflow/compute_output_node.hpp"

namespace vkit::workflow::node::op {

struct ChannelRemapParams {
  uint32_t outR{0};
  uint32_t outG{1};
  uint32_t outB{2};
  uint32_t outA{3};
};

class ChannelRemapNode : public workflow::ComputeOutputNode {
 public:
  ChannelRemapNode(std::string_view name, texture::TextureManager& mgr,
                   core::events::ComputeOutputBus& bus,
                   core::events::ComputeOutputResultBus& resultBus,
                   core::events::ComputeHandles handles);

  void execute() override;
  void setParams(ChannelRemapParams p);
  [[nodiscard]] auto getParams() const -> const ChannelRemapParams& {
    return params_;
  }

 private:
  ChannelRemapParams params_;
  graph::Pin* inImage_{nullptr};
};

}  // namespace vkit::workflow::node::op
