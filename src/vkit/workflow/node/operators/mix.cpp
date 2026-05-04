#include "vkit/workflow/node/operators/mix.hpp"

#include "vkit/core/events/operators.hpp"
#include "vkit/workflow/pin_type.hpp"

namespace vkit::workflow::node::op {

MixNode::MixNode(std::string_view name, texture::TextureManager& mgr,
                 core::events::ComputeOutputBus& bus,
                 core::events::ComputeOutputResultBus& resultBus,
                 core::events::ComputeHandles handles)
    : ComputeOutputNode(name, mgr, bus, resultBus, handles) {
  inA_ = addInputPin(pinKeyType(PinType::kColorTexture2D), "Image A");
  inB_ = addInputPin(pinKeyType(PinType::kColorTexture2D), "Image B");
  inFac_ = addInputPin(pinKeyType(PinType::kFloatTexture2D), "Factor");
  outImageF32_ =
      addOutputPin(pinKeyType(PinType::kFloatTexture2D), "Result (F32)");
  outColor_ =
      addOutputPin(pinKeyType(PinType::kColorTexture2D), "Result (Color)");
}

void MixNode::execute() {
  if (hasPendingJob()) return;
  auto tex_a = getConnectedTexture(inA_);
  auto tex_b = getConnectedTexture(inB_);
  if (!tex_a || !tex_b) {
    clearOutputs();
    return;
  }
  auto tex_fac = getConnectedTexture(inFac_);
  clearOutputs();
  core::events::MixPushConstants pc{
      .width = params_.width,
      .height = params_.height,
      .factor = params_.factor,
      .useFacTex = tex_fac ? 1U : 0U,
      .mode = static_cast<uint32_t>(params_.mode)};
  submitJob(pc.width, pc.height, &pc, sizeof(pc), tex_a, tex_b, tex_fac);
}

void MixNode::setParams(MixParams p) {
  params_ = p;
  markStale();
}

}  // namespace vkit::workflow::node::op
