#include "vkit/workflow/node/tint.hpp"

#include "vkit/graph/link.hpp"
#include "vkit/workflow/pin_type.hpp"

namespace vkit::workflow::node {

std::atomic<std::uint64_t> TintNode::request_counter_{0};

TintNode::TintNode(std::string_view name, core::events::TintJobBus& jobBus,
                   core::events::TintResultBus& resultBus,
                   texture::TextureManager& textureManager)
    : WorkflowNode(name),
      jobBus_{jobBus},
      textureManager_{textureManager},
      resultSub_{resultBus.subscribe([this](core::events::TintJobResult& ev) {
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
  inImage_ = addInputPin(pinKeyType(PinType::kColorTexture2D), "Color");
  outImageF32_ = addOutputPin(pinKeyType(PinType::kFloatTexture2D), "Image");
  outColor_ = addOutputPin(pinKeyType(PinType::kColorTexture2D), "Color");
}

TintNode::~TintNode() {
  if (outputF32Id.has_value()) textureManager_.remove(*outputF32Id);
  if (outputColorId.has_value()) textureManager_.remove(*outputColorId);
}

void TintNode::execute() {
  if (pendingRequestId_ != 0) return;

  auto links = inImage_->getLinks();
  if (links.empty()) {
    if (outputF32Id.has_value()) {
      textureManager_.remove(*outputF32Id);
      outputF32Id.reset();
    }
    if (outputColorId.has_value()) {
      textureManager_.remove(*outputColorId);
      outputColorId.reset();
    }
    if (outColor_) outColor_->clearData();
    if (outImageF32_) outImageF32_->clearData();
    setStatus(NodeStatus::kStale);
    propagateStale();
    return;
  }

  auto* source_pin = links.front()->getSrc();
  const auto* incoming_tex_id = source_pin->getData<std::uint32_t>();
  if (!incoming_tex_id) return;

  auto input_texture = textureManager_.get(*incoming_tex_id);
  if (!input_texture) {
    setStatus(NodeStatus::kError);
    return;
  }

  auto gfx_texture = input_texture->getGraphicsTexture();
  if (!gfx_texture) return;

  if (outputF32Id.has_value()) {
    textureManager_.remove(*outputF32Id);
    outputF32Id.reset();
  }
  if (outputColorId.has_value()) {
    textureManager_.remove(*outputColorId);
    outputColorId.reset();
  }
  if (outColor_) outColor_->clearData();
  if (outImageF32_) outImageF32_->clearData();

  pendingRequestId_ = ++request_counter_;
  setStatus(NodeStatus::kExecuting);

  core::events::TintPushConstants pc{
      .width = static_cast<uint32_t>(gfx_texture->getWidth()),
      .height = static_cast<uint32_t>(gfx_texture->getHeight()),
      .rCoef = params_.color[0],
      .gCoef = params_.color[1],
      .bCoef = params_.color[2],
      .aCoef = params_.color[3],
      .factor = params_.factor,
      .mode = static_cast<uint32_t>(params_.mode)};

  jobBus_.queueMessage(core::events::TintJobRequest{
      .requestId = pendingRequestId_,
      .inputTexture = input_texture,
      .params = pc,
  });
}

void TintNode::setParams(TintParams params) {
  params_ = params;
  pendingRequestId_ = 0;
  markStale();
}

};  // namespace vkit::workflow::node
