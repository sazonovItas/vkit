#include "vkit/workflow/node/operators/channel_adjust.hpp"

#include "vkit/core/events/operators.hpp"
#include "vkit/workflow/pin_type.hpp"

namespace vkit::workflow::node::op {

ChannelAdjustNode::ChannelAdjustNode(
    std::string_view name, texture::TextureManager& mgr,
    core::events::ComputeOutputBus& bus,
    core::events::ComputeOutputResultBus& resultBus,
    core::events::ComputeHandles handles)
    : ComputeOutputNode(name, mgr, bus, resultBus, handles) {
  inImage_     = addInputPin(pinKeyType(PinType::kColorTexture2D), "Image");
  outImageF32_ = addOutputPin(pinKeyType(PinType::kFloatTexture2D), "Image");
  outColor_    = addOutputPin(pinKeyType(PinType::kColorTexture2D), "Color");
}

void ChannelAdjustNode::execute() {
  if (hasPendingJob()) return;
  auto tex = getConnectedTexture(inImage_);
  if (!tex) { clearOutputs(); return; }
  clearOutputs();
  auto* gfx = tex->getGraphicsTexture().get();
  core::events::ChannelAdjustPushConstants pc{
      .width   = static_cast<uint32_t>(gfx->getWidth()),
      .height  = static_cast<uint32_t>(gfx->getHeight()),
      .gainR   = params_.gain[0],   .gainG  = params_.gain[1],
      .gainB   = params_.gain[2],   .gainA  = params_.gain[3],
      .biasR   = params_.bias[0],   .biasG  = params_.bias[1],
      .biasB   = params_.bias[2],   .biasA  = params_.bias[3],
      .invertR = params_.invert[0] ? 1u : 0u,
      .invertG = params_.invert[1] ? 1u : 0u,
      .invertB = params_.invert[2] ? 1u : 0u,
      .invertA = params_.invert[3] ? 1u : 0u,
  };
  submitJob(pc.width, pc.height, &pc, sizeof(pc), tex);
}

void ChannelAdjustNode::setParams(ChannelAdjustParams p) {
  params_ = p;
  markStale();
}

}  // namespace vkit::workflow::node::op
