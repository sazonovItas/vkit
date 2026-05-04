#include "vkit/workflow/compute_output_node.hpp"

#include "vkit/graph/link.hpp"

namespace vkit::workflow {

std::atomic<std::uint64_t> ComputeOutputNode::request_counter_{0};

ComputeOutputNode::ComputeOutputNode(
    std::string_view name, texture::TextureManager& mgr,
    core::events::ComputeOutputBus& jobBus,
    core::events::ComputeOutputResultBus& resultBus,
    core::events::ComputeHandles handles)
    : WorkflowNode(name),
      textureManager_{mgr},
      bus_{jobBus},
      handles_{handles},
      sub_{resultBus.subscribe([this](core::events::ComputeOutputResult& ev) {
        if (ev.requestId != pendingRequestId_) return;
        pendingRequestId_ = 0;

        if (!ev.error.empty()) {
          setStatus(NodeStatus::kError);
          return;
        }

        if (ev.imageF32 && ev.imageUnorm) {
          outputF32Id = textureManager_.add(ev.imageF32);
          outputColorId = textureManager_.add(ev.imageUnorm);

          if (outImageF32_ && outputF32Id)
            outImageF32_->setData<std::uint32_t>(*outputF32Id);
          if (outColor_ && outputColorId)
            outColor_->setData<std::uint32_t>(*outputColorId);

          setStatus(NodeStatus::kReady);
          propagateStale();
        }
      })} {}

ComputeOutputNode::~ComputeOutputNode() {
  if (outputF32Id.has_value()) textureManager_.remove(*outputF32Id);
  if (outputColorId.has_value()) textureManager_.remove(*outputColorId);
}

void ComputeOutputNode::submitJob(uint32_t w, uint32_t h, const void* pushData,
                                  uint32_t pushSize,
                                  std::shared_ptr<texture::Texture> inputA,
                                  std::shared_ptr<texture::Texture> inputB,
                                  std::shared_ptr<texture::Texture> inputC) {
  pendingRequestId_ = ++request_counter_;
  setStatus(NodeStatus::kExecuting);

  core::events::ComputeOutputJob job{
      .requestId = pendingRequestId_,
      .width = w,
      .height = h,
      .inputA = std::move(inputA),
      .inputB = std::move(inputB),
      .inputC = std::move(inputC),
      .handles = handles_,
      .pushData = std::vector<uint8_t>(
          static_cast<const uint8_t*>(pushData),
          static_cast<const uint8_t*>(pushData) + pushSize),
  };
  bus_.queueMessage(std::move(job));
}

auto ComputeOutputNode::getConnectedTexture(graph::Pin* pin) const
    -> std::shared_ptr<texture::Texture> {
  if (!pin || pin->getLinks().empty()) return nullptr;
  auto* src = pin->getLinks().front()->getSrc();
  const auto* id = src->getData<std::uint32_t>();
  if (!id) return nullptr;
  return textureManager_.get(*id);
}

void ComputeOutputNode::clearOutputs() {
  if (outputF32Id.has_value()) {
    textureManager_.remove(*outputF32Id);
    outputF32Id.reset();
  }
  if (outputColorId.has_value()) {
    textureManager_.remove(*outputColorId);
    outputColorId.reset();
  }
  if (outImageF32_) outImageF32_->clearData();
  if (outColor_) outColor_->clearData();
}

}  // namespace vkit::workflow
