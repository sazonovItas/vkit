#include "vkit/workflow/node/operators/channel_remap.hpp"

#include "vkit/core/events/operators.hpp"
#include "vkit/workflow/pin_type.hpp"

namespace vkit::workflow::node::op {

ChannelRemapNode::ChannelRemapNode(std::string_view name,
                                   texture::TextureManager& mgr,
                                   core::events::ComputeOutputBus& bus,
                                   core::events::ComputeOutputResultBus& resultBus,
                                   core::events::ComputeHandles handles)
    : ComputeOutputNode(name, mgr, bus, resultBus, handles) {
  inImage_     = addInputPin(pinKeyType(PinType::kColorTexture2D), "Image");
  outImageF32_ = addOutputPin(pinKeyType(PinType::kFloatTexture2D), "Image");
  outColor_    = addOutputPin(pinKeyType(PinType::kColorTexture2D), "Color");
}

void ChannelRemapNode::execute() {
  if (hasPendingJob()) return;
  auto tex = getConnectedTexture(inImage_);
  if (!tex) { clearOutputs(); return; }
  clearOutputs();
  auto* gfx = tex->getGraphicsTexture().get();
  core::events::ChannelRemapPushConstants pc{
      .width  = static_cast<uint32_t>(gfx->getWidth()),
      .height = static_cast<uint32_t>(gfx->getHeight()),
      .outR   = params_.outR,
      .outG   = params_.outG,
      .outB   = params_.outB,
      .outA   = params_.outA,
  };
  submitJob(pc.width, pc.height, &pc, sizeof(pc), tex);
}

void ChannelRemapNode::setParams(ChannelRemapParams p) {
  params_ = p;
  markStale();
}

}  // namespace vkit::workflow::node::op
