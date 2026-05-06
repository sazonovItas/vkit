#include "vkit/workflow/node/procedural/fractal_generator.hpp"

#include "vkit/core/events/fractal.hpp"
#include "vkit/workflow/pin_type.hpp"

namespace vkit::workflow::node::proc {

FractalGeneratorNode::FractalGeneratorNode(
    std::string_view name, texture::TextureManager& mgr,
    core::events::ComputeOutputBus& bus,
    core::events::ComputeOutputResultBus& resultBus,
    core::events::ComputeHandles handles)
    : ComputeOutputNode(name, mgr, bus, resultBus, handles) {
  outImageF32_ = addOutputPin(pinKeyType(PinType::kFloatTexture2D), "Image");
  outColor_ = addOutputPin(pinKeyType(PinType::kColorTexture2D), "Color");
}

void FractalGeneratorNode::execute() {
  if (hasPendingJob()) return;
  clearOutputs();
  core::events::FractalPushConstants pc{
      .width = params_.width,
      .height = params_.height,
      .fractalType = static_cast<uint32_t>(params_.type),
      .maxIterations = params_.maxIterations,
      .centerX = params_.centerX,
      .centerY = params_.centerY,
      .zoom = params_.zoom,
      .juliaRe = params_.juliaRe,
      .juliaIm = params_.juliaIm,
  };
  submitJob(pc.width, pc.height, &pc, sizeof(pc));
}

void FractalGeneratorNode::setParams(FractalParams params) {
  params_ = params;
  markStale();
}

}  // namespace vkit::workflow::node::proc
