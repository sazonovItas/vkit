#include "vkit/workflow/node/noise_generator.hpp"

#include "vkit/workflow/pin_type.hpp"

namespace vkit::workflow::node {

std::atomic<std::uint64_t> NoiseGeneratorNode::request_counter_{0};

NoiseGeneratorNode::NoiseGeneratorNode(std::string_view name,
                                       core::events::NoiseJobBus& jobBus,
                                       core::events::NoiseResultBus& resultBus,
                                       texture::TextureManager& textureManager)
    : WorkflowNode(name),
      jobBus_{jobBus},
      textureManager_{textureManager},
      resultSub_{resultBus.subscribe([this](core::events::NoiseJobResult& ev) {
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
  outColor_ = addOutputPin(pinKeyType(PinType::kColorTexture2D), "Color");
  outImageF32_ = addOutputPin(pinKeyType(PinType::kFloatTexture2D), "Image");
}

NoiseGeneratorNode::~NoiseGeneratorNode() {
  if (outputF32Id.has_value()) textureManager_.remove(*outputF32Id);
  if (outputColorId.has_value()) textureManager_.remove(*outputColorId);
}

void NoiseGeneratorNode::execute() {
  if (pendingRequestId_ != 0) return;

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

  core::events::NoisePushConstants pc{
      .width = params_.width,
      .height = params_.height,
      .noiseType = static_cast<uint32_t>(params_.type),
      .worleyMode = static_cast<uint32_t>(params_.worleyMode),
      .scale = params_.scale,
      .offsetX = params_.offsetX,
      .offsetY = params_.offsetY,
      .seed = params_.seed,
      .octaves = params_.octaves,
      .persistence = params_.persistence,
      .lacunarity = params_.lacunarity,
      .worleyJitter = params_.worleyJitter,
  };

  jobBus_.queueMessage(core::events::NoiseJobRequest{
      .requestId = pendingRequestId_,
      .params = pc,
  });
}

void NoiseGeneratorNode::setParams(NoiseParams params) {
  params_ = params;
  pendingRequestId_ = 0;
  markStale();
}

};  // namespace vkit::workflow::node
