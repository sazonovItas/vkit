#pragma once

#include <string_view>

#include "vkit/core/events/compute_output.hpp"
#include "vkit/workflow/compute_output_node.hpp"

namespace vkit::workflow::node::proc {

enum class NoiseType : uint32_t { kGradient = 0, kFbm = 1, kWorley = 2 };
enum class WorleyMode : uint32_t { kF1 = 0, kF2 = 1, kF2F1 = 2 };

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

class NoiseGeneratorNode : public workflow::ComputeOutputNode {
 public:
  NoiseGeneratorNode(std::string_view name, texture::TextureManager& mgr,
                     core::events::ComputeOutputBus& bus,
                     core::events::ComputeOutputResultBus& resultBus,
                     core::events::ComputeHandles handles);

  void execute() override;
  void setParams(NoiseParams params);
  [[nodiscard]] auto getParams() const -> const NoiseParams& { return params_; }

 private:
  NoiseParams params_;
};

}  // namespace vkit::workflow::node::proc
