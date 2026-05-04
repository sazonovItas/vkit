#pragma once

#include <string_view>

#include "vkit/core/events/compute_output.hpp"
#include "vkit/workflow/compute_output_node.hpp"

namespace vkit::workflow::node::op {

struct NormalMapParams {
  float strength{1.0F};
  uint32_t invertX{0};
  uint32_t invertY{0};
};

class NormalMapNode : public workflow::ComputeOutputNode {
 public:
  NormalMapNode(std::string_view name, texture::TextureManager& mgr,
                core::events::ComputeOutputBus& bus,
                core::events::ComputeOutputResultBus& resultBus,
                core::events::ComputeHandles handles);

  void execute() override;
  void setParams(NormalMapParams p);
  [[nodiscard]] auto getParams() const -> const NormalMapParams& {
    return params_;
  }

 private:
  NormalMapParams params_;
  graph::Pin* inImage_{nullptr};
};

}  // namespace vkit::workflow::node::op
