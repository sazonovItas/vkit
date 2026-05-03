#include "vkit/workflow/node/mix.hpp"

#include "vkit/graph/link.hpp"
#include "vkit/workflow/pin_type.hpp"

namespace vkit::workflow::node {

std::atomic<std::uint64_t> MixNode::request_counter_{0};

MixNode::MixNode(std::string_view name, core::events::MixJobBus& jobBus,
                 core::events::MixResultBus& resultBus,
                 texture::TextureManager& textureManager)
    : WorkflowNode(name),
      jobBus_{jobBus},
      textureManager_{textureManager},
      resultSub_{resultBus.subscribe([this](core::events::MixJobResult& ev) {
        if (ev.requestId != pendingRequestId_) return;
        pendingRequestId_ = 0;
        if (!ev.error.empty()) {
          setStatus(NodeStatus::kError);
          return;
        }
        if (ev.imageF32 && ev.imageUnorm) {
          outputF32Id = textureManager_.add(ev.imageF32);
          outputColorId = textureManager_.add(ev.imageUnorm);
          if (outputF32Id) outImageF32_->setData<std::uint32_t>(*outputF32Id);
          if (outputColorId) outColor_->setData<std::uint32_t>(*outputColorId);
          setStatus(NodeStatus::kReady);
          propagateStale();
        }
      })} {
  inA_ = addInputPin(pinKeyType(PinType::kColorTexture2D), "Image A");
  inB_ = addInputPin(pinKeyType(PinType::kColorTexture2D), "Image B");
  inFac_ = addInputPin(pinKeyType(PinType::kFloatTexture2D), "Factor (Mask)");

  outImageF32_ =
      addOutputPin(pinKeyType(PinType::kFloatTexture2D), "Result (F32)");
  outColor_ =
      addOutputPin(pinKeyType(PinType::kColorTexture2D), "Result (Color)");
}

MixNode::~MixNode() {
  if (outputF32Id.has_value()) textureManager_.remove(*outputF32Id);
  if (outputColorId.has_value()) textureManager_.remove(*outputColorId);
}

void MixNode::execute() {
  if (pendingRequestId_ != 0) return;

  if (inA_->getLinks().empty() || inB_->getLinks().empty()) {
    if (outputF32Id.has_value()) {
      textureManager_.remove(*outputF32Id);
      outputF32Id.reset();
    }
    if (outputColorId.has_value()) {
      textureManager_.remove(*outputColorId);
      outputColorId.reset();
    }
    outColor_->clearData();
    outImageF32_->clearData();
    setStatus(NodeStatus::kStale);
    propagateStale();
    return;
  }

  const auto* id_a =
      inA_->getLinks().front()->getSrc()->getData<std::uint32_t>();
  const auto* id_b =
      inB_->getLinks().front()->getSrc()->getData<std::uint32_t>();
  if (!id_a || !id_b) return;

  auto tex_a = textureManager_.get(*id_a);
  auto tex_b = textureManager_.get(*id_b);
  if (!tex_a || !tex_b) {
    setStatus(NodeStatus::kError);
    return;
  }

  std::shared_ptr<texture::Texture> tex_fac = nullptr;
  if (!inFac_->getLinks().empty()) {
    const auto* id_fac =
        inFac_->getLinks().front()->getSrc()->getData<std::uint32_t>();

    if (!id_fac) {
      return;
    }

    tex_fac = textureManager_.get(*id_fac);
    if (!tex_fac) {
      setStatus(NodeStatus::kError);
      return;
    }
  }

  if (outputF32Id.has_value()) {
    textureManager_.remove(*outputF32Id);
    outputF32Id.reset();
  }
  if (outputColorId.has_value()) {
    textureManager_.remove(*outputColorId);
    outputColorId.reset();
  }
  outColor_->clearData();
  outImageF32_->clearData();

  pendingRequestId_ = ++request_counter_;
  setStatus(NodeStatus::kExecuting);

  core::events::MixPushConstants pc{
      .width = params_.width,
      .height = params_.height,
      .factor = params_.factor,
      .useFacTex = tex_fac ? 1U : 0U,
      .mode = static_cast<uint32_t>(params_.mode)};

  jobBus_.queueMessage(core::events::MixJobRequest{
      .requestId = pendingRequestId_,
      .inputA = tex_a,
      .inputB = tex_b,
      .inputFac = tex_fac,
      .params = pc,
  });
}

void MixNode::setParams(MixParams params) {
  params_ = params;
  pendingRequestId_ = 0;
  markStale();
}

};  // namespace vkit::workflow::node
