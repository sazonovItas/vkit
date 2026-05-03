#pragma once

#include <atomic>
#include <optional>
#include <string_view>

#include "vkit/core/events/operators.hpp"
#include "vkit/message_bus/message_bus.hpp"
#include "vkit/texture/manager.hpp"
#include "vkit/workflow/workflow_node.hpp"

namespace vkit::workflow::node {

struct SobelParams {
  float intensity{1.0F};
  float threshold{0.1F};
};

class SobelNode : public WorkflowNode {
 public:
  SobelNode(std::string_view name, core::events::SobelJobBus& jobBus,
            core::events::SobelResultBus& resultBus,
            texture::TextureManager& textureManager);
  ~SobelNode() override;

  void execute() override;

  void setParams(SobelParams params);
  [[nodiscard]] auto getParams() const -> const SobelParams& { return params_; }

  std::optional<std::uint32_t> outputF32Id;
  std::optional<std::uint32_t> outputColorId;

 private:
  core::events::SobelJobBus& jobBus_;
  texture::TextureManager& textureManager_;

  message_bus::Subscription<core::events::SobelJobResult> resultSub_;

  SobelParams params_;
  std::uint64_t pendingRequestId_{0};

  graph::Pin* inImage_{nullptr};
  graph::Pin* outImageF32_{nullptr};
  graph::Pin* outColor_{nullptr};

  static std::atomic<std::uint64_t> request_counter_;
};

};  // namespace vkit::workflow::node
