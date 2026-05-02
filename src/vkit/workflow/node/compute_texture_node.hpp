#pragma once

#include <atomic>
#include <cstdint>
#include <memory>
#include <optional>
#include <string_view>

#include "vkit/compute/async_compute.hpp"
#include "vkit/core/events/compute.hpp"
#include "vkit/graph/link.hpp"
#include "vkit/graph/pin.hpp"
#include "vkit/texture/texture.hpp"
#include "vkit/workflow/workflow_node.hpp"

namespace vkit::workflow::node {

class ComputeTextureNode : public WorkflowNode {
 public:
  ComputeTextureNode(std::string_view name, core::events::ComputeJobBus& jobBus,
                     core::events::ComputeResultBus& resultBus);
  ~ComputeTextureNode() override = default;

  std::optional<std::uint32_t> outputTextureId;

 protected:
  void dispatchCompute(std::shared_ptr<compute::ComputeTask> task,
                       std::shared_ptr<texture::Texture> output_texture);

  template <typename NodeT>
  auto getUpstreamNode(const graph::Pin& input_pin) -> NodeT* {
    const auto& links = input_pin.getLinks();
    if (links.empty()) return nullptr;
    return dynamic_cast<NodeT*>(links.front()->getSrc()->getOwnerNode());
  }

 private:
  core::events::ComputeJobBus& jobBus_;
  message_bus::Subscription<core::events::ComputeJobResult> resultSub_;

  std::uint64_t pendingRequestId_{0};
  static std::atomic<std::uint64_t> request_counter_;
};

};  // namespace vkit::workflow::node
