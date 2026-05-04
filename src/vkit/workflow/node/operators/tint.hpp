#pragma once

#include <string_view>

#include "vkit/core/events/compute_output.hpp"
#include "vkit/core/events/operators.hpp"
#include "vkit/workflow/compute_output_node.hpp"

namespace vkit::workflow::node::op {

enum class TintMode : uint32_t { kMix = 0, kMultiply = 1, kAdd = 2, kScreen = 3 };

struct TintParams {
  float    color[4]{1.0F, 1.0F, 1.0F, 1.0F};
  float    factor{1.0F};
  TintMode mode{TintMode::kMultiply};
};

class TintNode : public workflow::ComputeOutputNode {
 public:
  TintNode(std::string_view name, texture::TextureManager& mgr,
           core::events::ComputeOutputBus&       bus,
           core::events::ComputeOutputResultBus& resultBus,
           core::events::ComputeHandles          handles);

  void execute() override;
  void setParams(TintParams p);
  [[nodiscard]] auto getParams() const -> const TintParams& { return params_; }

 private:
  TintParams  params_;
  graph::Pin* inImage_{nullptr};
};

}  // namespace vkit::workflow::node::op
