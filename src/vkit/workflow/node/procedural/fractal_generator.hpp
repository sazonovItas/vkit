#pragma once

#include <string_view>

#include "vkit/core/events/compute_output.hpp"
#include "vkit/workflow/compute_output_node.hpp"

namespace vkit::workflow::node::proc {

enum class FractalType : uint32_t {
  kMandelbrot = 0,
  kJulia = 1,
  kBurningShip = 2,
};

struct FractalParams {
  FractalType type{FractalType::kMandelbrot};
  uint32_t width{512};
  uint32_t height{512};
  int32_t maxIterations{256};
  float centerX{-0.5F};
  float centerY{0.0F};
  float zoom{1.0F};
  float juliaRe{-0.7F};
  float juliaIm{0.27F};
};

class FractalGeneratorNode : public workflow::ComputeOutputNode {
 public:
  FractalGeneratorNode(std::string_view name, texture::TextureManager& mgr,
                       core::events::ComputeOutputBus& bus,
                       core::events::ComputeOutputResultBus& resultBus,
                       core::events::ComputeHandles handles);

  void execute() override;
  void setParams(FractalParams params);
  [[nodiscard]] auto getParams() const -> const FractalParams& {
    return params_;
  }

 private:
  FractalParams params_;
};

}  // namespace vkit::workflow::node::proc
