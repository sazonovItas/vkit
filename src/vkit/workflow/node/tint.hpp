#pragma once

#include <atomic>
#include <optional>
#include <string_view>

#include "vkit/core/events/operators.hpp"
#include "vkit/message_bus/message_bus.hpp"
#include "vkit/texture/manager.hpp"
#include "vkit/workflow/workflow_node.hpp"

namespace vkit::workflow::node {

enum class TintMode : uint32_t {
  kMix = 0,
  kMultiply = 1,
  kAdd = 2,
  kScreen = 3,
};

struct TintParams {
  float color[4] = {1.0F, 1.0F, 1.0F, 1.0F};
  float factor{1.0F};
  TintMode mode{TintMode::kMultiply};
};

class TintNode : public WorkflowNode {
 public:
  TintNode(std::string_view name, core::events::TintJobBus& jobBus,
           core::events::TintResultBus& resultBus,
           texture::TextureManager& textureManager);
  ~TintNode() override;

  void execute() override;
  void setParams(TintParams params);
  [[nodiscard]] auto getParams() const -> const TintParams& { return params_; }

  std::optional<std::uint32_t> outputF32Id;
  std::optional<std::uint32_t> outputColorId;

 private:
  core::events::TintJobBus& jobBus_;
  texture::TextureManager& textureManager_;
  message_bus::Subscription<core::events::TintJobResult> resultSub_;
  TintParams params_;
  std::uint64_t pendingRequestId_{0};

  graph::Pin* inImage_{nullptr};
  graph::Pin* outImageF32_{nullptr};
  graph::Pin* outColor_{nullptr};

  static std::atomic<std::uint64_t> request_counter_;
};

};  // namespace vkit::workflow::node
