#pragma once

#include <atomic>
#include <optional>
#include <string_view>

#include "vkit/core/events/operators.hpp"
#include "vkit/message_bus/message_bus.hpp"
#include "vkit/texture/manager.hpp"
#include "vkit/workflow/workflow_node.hpp"

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

class MixNode : public WorkflowNode {
 public:
  MixNode(std::string_view name, core::events::MixJobBus& jobBus,
          core::events::MixResultBus& resultBus,
          texture::TextureManager& textureManager);
  ~MixNode() override;

  void execute() override;
  void setParams(MixParams params);
  [[nodiscard]] auto getParams() const -> const MixParams& { return params_; }

  std::optional<std::uint32_t> outputF32Id;
  std::optional<std::uint32_t> outputColorId;

 private:
  core::events::MixJobBus& jobBus_;
  texture::TextureManager& textureManager_;
  message_bus::Subscription<core::events::MixJobResult> resultSub_;

  MixParams params_;
  std::uint64_t pendingRequestId_{0};

  graph::Pin* inA_{nullptr};
  graph::Pin* inB_{nullptr};
  graph::Pin* inFac_{nullptr};

  graph::Pin* outImageF32_{nullptr};
  graph::Pin* outColor_{nullptr};

  static std::atomic<std::uint64_t> request_counter_;
};

};  // namespace vkit::workflow::node::op
