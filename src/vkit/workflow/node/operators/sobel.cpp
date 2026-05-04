#include "vkit/workflow/node/operators/sobel.hpp"

#include "vkit/core/events/operators.hpp"
#include "vkit/workflow/pin_type.hpp"

namespace vkit::workflow::node::op {

SobelNode::SobelNode(std::string_view name, texture::TextureManager& mgr,
                     core::events::ComputeOutputBus& bus,
                     core::events::ComputeOutputResultBus& resultBus,
                     core::events::ComputeHandles handles)
    : ComputeOutputNode(name, mgr, bus, resultBus, handles) {
  inImage_ = addInputPin(pinKeyType(PinType::kColorTexture2D), "Image");
  outImageF32_ = addOutputPin(pinKeyType(PinType::kFloatTexture2D), "Image");
  outColor_ = addOutputPin(pinKeyType(PinType::kColorTexture2D), "Color");
}

void SobelNode::execute() {
  if (hasPendingJob()) return;
  auto tex = getConnectedTexture(inImage_);
  if (!tex) {
    clearOutputs();
    return;
  }
  clearOutputs();
  auto* gfx = tex->getGraphicsTexture().get();
  core::events::SobelPushConstants pc{
      .width = static_cast<uint32_t>(gfx->getWidth()),
      .height = static_cast<uint32_t>(gfx->getHeight()),
      .intensity = params_.intensity,
      .threshold = params_.threshold};
  submitJob(pc.width, pc.height, &pc, sizeof(pc), tex);
}

void SobelNode::setParams(SobelParams p) {
  params_ = p;
  markStale();
}

}  // namespace vkit::workflow::node::op
