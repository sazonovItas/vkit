#include "vkit/workflow/node/operators/tint.hpp"
#include "vkit/workflow/pin_type.hpp"

namespace vkit::workflow::node::op {

TintNode::TintNode(std::string_view name, texture::TextureManager& mgr,
                   core::events::ComputeOutputBus& bus,
                   core::events::ComputeOutputResultBus& resultBus,
                   core::events::ComputeHandles handles)
    : ComputeOutputNode(name, mgr, bus, resultBus, handles) {
  inImage_     = addInputPin(pinKeyType(PinType::kColorTexture2D), "Color");
  outImageF32_ = addOutputPin(pinKeyType(PinType::kFloatTexture2D), "Image");
  outColor_    = addOutputPin(pinKeyType(PinType::kColorTexture2D), "Color");
}

void TintNode::execute() {
  if (hasPendingJob()) return;
  auto tex = getConnectedTexture(inImage_);
  if (!tex) { clearOutputs(); return; }
  clearOutputs();
  auto* gfx = tex->getGraphicsTexture().get();
  core::events::TintPushConstants pc{
      .width = (uint32_t)gfx->getWidth(), .height = (uint32_t)gfx->getHeight(),
      .rCoef = params_.color[0], .gCoef = params_.color[1],
      .bCoef = params_.color[2], .aCoef = params_.color[3],
      .factor = params_.factor, .mode = static_cast<uint32_t>(params_.mode)};
  submitJob(pc.width, pc.height, &pc, sizeof(pc), tex);
}

void TintNode::setParams(TintParams p) { params_ = p; markStale(); }

}  // namespace vkit::workflow::node::op
