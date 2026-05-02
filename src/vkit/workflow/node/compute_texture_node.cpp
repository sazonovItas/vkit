#include "vkit/workflow/node/compute_texture_node.hpp"

namespace vkit::workflow::node {

std::atomic<std::uint64_t> ComputeTextureNode::request_counter_{0};

ComputeTextureNode::ComputeTextureNode(
    std::string_view name, core::events::ComputeJobBus& jobBus,
    core::events::ComputeResultBus& resultBus)
    : WorkflowNode{name},
      jobBus_{jobBus},
      resultSub_{
          resultBus.subscribe([this](core::events::ComputeJobResult& result) {
            if (result.requestId != pendingRequestId_) return;
            pendingRequestId_ = 0;
            if (result.texture) {
              if (auto id = result.texture->getStorageId()) {
                outputTextureId = id;
              }
            }
            setStatus(NodeStatus::kReady);
            propagateStale();
          })} {}

void ComputeTextureNode::dispatchCompute(
    std::shared_ptr<compute::ComputeTask> task,
    std::shared_ptr<texture::Texture> output_texture) {
  pendingRequestId_ = ++request_counter_;
  setStatus(NodeStatus::kExecuting);
  jobBus_.queueMessage(core::events::ComputeJobRequest{
      .requestId = pendingRequestId_,
      .task = std::move(task),
      .outputTexture = std::move(output_texture),
  });
}

};  // namespace vkit::workflow::node
