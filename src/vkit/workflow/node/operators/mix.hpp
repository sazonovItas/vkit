#pragma once

#include <string_view>

#include "vkit/core/events/compute_output.hpp"
#include "vkit/workflow/compute_output_node.hpp"

namespace vkit::workflow::node::op {

enum class MixMode : uint32_t {
  kMix = 0,
  kMultiply = 1,
  kAdd = 2,
  kScreen = 3
};

struct MixParams {
  uint32_t width{512};
  uint32_t height{512};
  float factor{0.5F};
  MixMode mode{MixMode::kMix};
};

class MixNode : public workflow::ComputeOutputNode {
 public:
  MixNode(std::string_view name, texture::TextureManager& mgr,
          core::events::ComputeOutputBus& bus,
          core::events::ComputeOutputResultBus& resultBus,
          core::events::ComputeHandles handles);

  void execute() override;
  void setParams(MixParams p);
  [[nodiscard]] auto getParams() const -> const MixParams& { return params_; }

 private:
  MixParams params_;
  graph::Pin* inA_{nullptr};
  graph::Pin* inB_{nullptr};
  graph::Pin* inFac_{nullptr};
};

}  // namespace vkit::workflow::node::op
