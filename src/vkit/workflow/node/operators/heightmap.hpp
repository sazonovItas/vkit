#pragma once

#include <atomic>
#include <optional>
#include <string_view>

#include "vkit/core/events/operators.hpp"
#include "vkit/message_bus/message_bus.hpp"
#include "vkit/texture/manager.hpp"
#include "vkit/workflow/workflow_node.hpp"

namespace vkit::workflow::node::op {

struct HeightMapParams {
  float contrast{1.0F};
  float brightness{0.0F};
  uint32_t invert{0};
};

class HeightMapNode : public WorkflowNode {
 public:
  HeightMapNode(std::string_view name, core::events::HeightMapJobBus& jobBus,
                core::events::HeightMapResultBus& resultBus,
                texture::TextureManager& textureManager);
  ~HeightMapNode() override;

  void execute() override;

  void setParams(HeightMapParams params);
  [[nodiscard]] auto getParams() const -> const HeightMapParams& {
    return params_;
  }

  std::optional<std::uint32_t> outputF32Id;
  std::optional<std::uint32_t> outputColorId;

 private:
  core::events::HeightMapJobBus& jobBus_;
  texture::TextureManager& textureManager_;

  message_bus::Subscription<core::events::HeightMapJobResult> resultSub_;

  HeightMapParams params_;
  std::uint64_t pendingRequestId_{0};

  graph::Pin* inImage_{nullptr};
  graph::Pin* outImageF32_{nullptr};
  graph::Pin* outColor_{nullptr};

  static std::atomic<std::uint64_t> request_counter_;
};

};  // namespace vkit::workflow::node::op
