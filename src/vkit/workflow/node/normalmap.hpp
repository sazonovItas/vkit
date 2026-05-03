#pragma once

#include <atomic>
#include <optional>
#include <string_view>

#include "vkit/core/events/operators.hpp"
#include "vkit/message_bus/message_bus.hpp"
#include "vkit/texture/manager.hpp"
#include "vkit/workflow/workflow_node.hpp"

namespace vkit::workflow::node {

struct NormalMapParams {
  float strength{1.0F};
  uint32_t invertX{0};
  uint32_t invertY{0};
};

class NormalMapNode : public WorkflowNode {
 public:
  NormalMapNode(std::string_view name, core::events::NormalMapJobBus& jobBus,
                core::events::NormalMapResultBus& resultBus,
                texture::TextureManager& textureManager);
  ~NormalMapNode() override;

  void execute() override;
  void setParams(NormalMapParams params);
  [[nodiscard]] auto getParams() const -> const NormalMapParams& {
    return params_;
  }

  std::optional<std::uint32_t> outputF32Id;
  std::optional<std::uint32_t> outputColorId;

 private:
  core::events::NormalMapJobBus& jobBus_;
  texture::TextureManager& textureManager_;
  message_bus::Subscription<core::events::NormalMapJobResult> resultSub_;

  NormalMapParams params_;
  std::uint64_t pendingRequestId_{0};

  graph::Pin* inImage_{nullptr};
  graph::Pin* outImageF32_{nullptr};
  graph::Pin* outColor_{nullptr};

  static std::atomic<std::uint64_t> request_counter_;
};

};  // namespace vkit::workflow::node
