#pragma once

#include <string_view>

#include "vkit/core/events/compute_output.hpp"
#include "vkit/workflow/compute_output_node.hpp"

namespace vkit::workflow::node::op {

struct HeightMapParams {
  float contrast{1.0F};
  float brightness{0.0F};
  uint32_t invert{0};
};

class HeightMapNode : public workflow::ComputeOutputNode {
 public:
  HeightMapNode(std::string_view name, texture::TextureManager& mgr,
                core::events::ComputeOutputBus& bus,
                core::events::ComputeOutputResultBus& resultBus,
                core::events::ComputeHandles handles);

  void execute() override;
  void setParams(HeightMapParams p);
  [[nodiscard]] auto getParams() const -> const HeightMapParams& {
    return params_;
  }

 private:
  HeightMapParams params_;
  graph::Pin* inImage_{nullptr};
};

}  // namespace vkit::workflow::node::op
