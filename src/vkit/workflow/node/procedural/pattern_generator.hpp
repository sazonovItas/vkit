#pragma once

#include <string_view>

#include "vkit/workflow/compute_output_node.hpp"

namespace vkit::workflow::node::proc {

enum class PatternType : uint32_t { kBrick = 0, kWood = 1, kChain = 2 };

struct PatternParams {
  uint32_t width{512};
  uint32_t height{512};
  PatternType type{PatternType::kBrick};

  float scale{5.0F};
  float thickness{0.05F};
  float smoothness{0.02F};
  float param1{0.5F};
  float param2{0.0F};
};

class PatternGeneratorNode : public ComputeOutputNode {
 public:
  PatternGeneratorNode(std::string_view name, texture::TextureManager& mgr,
                       core::events::ComputeOutputBus& bus,
                       core::events::ComputeOutputResultBus& resultBus,
                       core::events::ComputeHandles handles);

  void execute() override;

  [[nodiscard]] auto getParams() const -> const PatternParams& {
    return params_;
  }
  void setParams(PatternParams params);

 private:
  PatternParams params_;
};

}  // namespace vkit::workflow::node::proc
