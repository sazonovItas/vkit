#pragma once

#include <atomic>
#include <optional>
#include <string_view>

#include "vkit/core/events/noise.hpp"
#include "vkit/message_bus/message_bus.hpp"
#include "vkit/texture/manager.hpp"
#include "vkit/workflow/workflow_node.hpp"

namespace vkit::workflow::node {

enum class NoiseType : uint32_t {
  kGradient = 0,
  kFbm = 1,
  kWorley = 2,
};

enum class WorleyMode : uint32_t {
  kF1 = 0,
  kF2 = 1,
  kF2F1 = 2,
};

struct NoiseParams {
  NoiseType type{NoiseType::kGradient};
  uint32_t width{512};
  uint32_t height{512};
  float scale{4.0F};
  float offsetX{0.0F};
  float offsetY{0.0F};
  float seed{0.0F};
  int32_t octaves{6};
  float persistence{0.5F};
  float lacunarity{2.0F};
  WorleyMode worleyMode{WorleyMode::kF1};
  float worleyJitter{1.0F};
};

class NoiseGeneratorNode : public WorkflowNode {
 public:
  NoiseGeneratorNode(std::string_view name, core::events::NoiseJobBus& jobBus,
                     core::events::NoiseResultBus& resultBus,
                     texture::TextureManager& textureManager);
  ~NoiseGeneratorNode() override;

  void execute() override;

  void setParams(NoiseParams params);
  [[nodiscard]] auto getParams() const -> const NoiseParams& { return params_; }

  std::optional<std::uint32_t> outputF32Id;
  std::optional<std::uint32_t> outputColorId;

 private:
  core::events::NoiseJobBus& jobBus_;
  texture::TextureManager& textureManager_;

  message_bus::Subscription<core::events::NoiseJobResult> resultSub_;

  NoiseParams params_;
  std::uint64_t pendingRequestId_{0};

  graph::Pin* outImageF32_{nullptr};
  graph::Pin* outColor_{nullptr};

  static std::atomic<std::uint64_t> request_counter_;
};

};  // namespace vkit::workflow::node
