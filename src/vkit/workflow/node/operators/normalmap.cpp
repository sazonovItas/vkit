#include "vkit/workflow/node/operators/normalmap.hpp"

#include "vkit/core/events/operators.hpp"
#include "vkit/workflow/pin_type.hpp"

namespace vkit::workflow::node::op {

NormalMapNode::NormalMapNode(std::string_view name,
                             texture::TextureManager& mgr,
                             core::events::ComputeOutputBus& bus,
                             core::events::ComputeOutputResultBus& resultBus,
                             core::events::ComputeHandles handles)
    : ComputeOutputNode(name, mgr, bus, resultBus, handles) {
  inImage_ = addInputPin(pinKeyType(PinType::kColorTexture2D), "Color");
  outImageF32_ = addOutputPin(pinKeyType(PinType::kFloatTexture2D), "Image");
  outColor_ = addOutputPin(pinKeyType(PinType::kColorTexture2D), "Color");
}

void NormalMapNode::execute() {
  if (hasPendingJob()) return;
  auto tex = getConnectedTexture(inImage_);
  if (!tex) {
    clearOutputs();
    return;
  }
  clearOutputs();
  auto* gfx = tex->getGraphicsTexture().get();
  core::events::NormalMapPushConstants pc{
      .width = static_cast<uint32_t>(gfx->getWidth()),
      .height = static_cast<uint32_t>(gfx->getHeight()),
      .strength = params_.strength,
      .invertX = params_.invertX,
      .invertY = params_.invertY};
  submitJob(pc.width, pc.height, &pc, sizeof(pc), tex);
}

void NormalMapNode::setParams(NormalMapParams p) {
  params_ = p;
  markStale();
}

}  // namespace vkit::workflow::node::op
