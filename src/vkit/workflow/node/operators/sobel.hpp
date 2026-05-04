#pragma once

#include <string_view>

#include "vkit/core/events/compute_output.hpp"
#include "vkit/workflow/compute_output_node.hpp"

namespace vkit::workflow::node::op {

struct SobelParams {
  float intensity{1.0F};
  float threshold{0.1F};
};

class SobelNode : public workflow::ComputeOutputNode {
 public:
  SobelNode(std::string_view name, texture::TextureManager& mgr,
            core::events::ComputeOutputBus& bus,
            core::events::ComputeOutputResultBus& resultBus,
            core::events::ComputeHandles handles);

  void execute() override;
  void setParams(SobelParams p);
  [[nodiscard]] auto getParams() const -> const SobelParams& { return params_; }

 private:
  SobelParams params_;
  graph::Pin* inImage_{nullptr};
};

}  // namespace vkit::workflow::node::op
