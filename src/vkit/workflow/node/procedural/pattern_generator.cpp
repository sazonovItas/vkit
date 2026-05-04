#include "vkit/workflow/node/procedural/pattern_generator.hpp"

#include "vkit/core/events/pattern.hpp"
#include "vkit/workflow/pin_type.hpp"

namespace vkit::workflow::node::proc {

PatternGeneratorNode::PatternGeneratorNode(
    std::string_view name, texture::TextureManager& mgr,
    core::events::ComputeOutputBus& bus,
    core::events::ComputeOutputResultBus& resultBus,
    core::events::ComputeHandles handles)
    : ComputeOutputNode(name, mgr, bus, resultBus, handles) {
  outImageF32_ = addOutputPin(pinKeyType(PinType::kFloatTexture2D), "Image");
  outColor_ = addOutputPin(pinKeyType(PinType::kColorTexture2D), "Color");
}

void PatternGeneratorNode::execute() {
  if (hasPendingJob()) return;
  clearOutputs();

  core::events::PatternPushConstants pc{
      .width = params_.width,
      .height = params_.height,
      .patternType = static_cast<uint32_t>(params_.type),
      .scale = params_.scale,
      .thickness = params_.thickness,
      .smoothness = params_.smoothness,
      .param1 = params_.param1,
      .param2 = params_.param2,
  };

  submitJob(pc.width, pc.height, &pc, sizeof(pc));
}

void PatternGeneratorNode::setParams(PatternParams params) {
  params_ = params;
  markStale();
}

}  // namespace vkit::workflow::node::proc
