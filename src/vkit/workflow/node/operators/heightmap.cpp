#include "vkit/workflow/node/operators/heightmap.hpp"

#include "vkit/core/events/operators.hpp"
#include "vkit/workflow/pin_type.hpp"

namespace vkit::workflow::node::op {

HeightMapNode::HeightMapNode(std::string_view name,
                             texture::TextureManager& mgr,
                             core::events::ComputeOutputBus& bus,
                             core::events::ComputeOutputResultBus& resultBus,
                             core::events::ComputeHandles handles)
    : ComputeOutputNode(name, mgr, bus, resultBus, handles) {
  inImage_ = addInputPin(pinKeyType(PinType::kColorTexture2D), "Color");
  outImageF32_ = addOutputPin(pinKeyType(PinType::kFloatTexture2D), "Image");
  outColor_ = addOutputPin(pinKeyType(PinType::kColorTexture2D), "Color");
}

void HeightMapNode::execute() {
  if (hasPendingJob()) return;
  auto tex = getConnectedTexture(inImage_);
  if (!tex) {
    clearOutputs();
    return;
  }
  clearOutputs();
  auto* gfx = tex->getGraphicsTexture().get();
  core::events::HeightMapPushConstants pc{
      .width = static_cast<uint32_t>(gfx->getWidth()),
      .height = static_cast<uint32_t>(gfx->getHeight()),
      .contrast = params_.contrast,
      .brightness = params_.brightness,
      .invert = params_.invert};
  submitJob(pc.width, pc.height, &pc, sizeof(pc), tex);
}

void HeightMapNode::setParams(HeightMapParams p) {
  params_ = p;
  markStale();
}

}  // namespace vkit::workflow::node::op
