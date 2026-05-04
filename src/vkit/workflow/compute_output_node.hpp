#pragma once

#include <atomic>
#include <memory>
#include <optional>

#include "vkit/core/events/compute_output.hpp"
#include "vkit/graph/pin.hpp"
#include "vkit/message_bus/message_bus.hpp"
#include "vkit/texture/manager.hpp"
#include "vkit/workflow/workflow_node.hpp"

namespace vkit::workflow {

class ComputeOutputNode : public WorkflowNode {
 public:
  ComputeOutputNode(std::string_view name, texture::TextureManager& mgr,
                    core::events::ComputeOutputBus& jobBus,
                    core::events::ComputeOutputResultBus& resultBus,
                    core::events::ComputeHandles handles);
  ~ComputeOutputNode() override;

  std::optional<std::uint32_t> outputF32Id;
  std::optional<std::uint32_t> outputColorId;

 protected:
  void submitJob(uint32_t w, uint32_t h, const void* pushData,
                 uint32_t pushSize,
                 std::shared_ptr<texture::Texture> inputA = nullptr,
                 std::shared_ptr<texture::Texture> inputB = nullptr,
                 std::shared_ptr<texture::Texture> inputC = nullptr);

  [[nodiscard]] bool hasPendingJob() const noexcept {
    return pendingRequestId_ != 0;
  }

  [[nodiscard]] auto getConnectedTexture(graph::Pin* pin) const
      -> std::shared_ptr<texture::Texture>;

  void clearOutputs();

  graph::Pin* outImageF32_{nullptr};
  graph::Pin* outColor_{nullptr};
  texture::TextureManager& textureManager_;

 private:
  core::events::ComputeOutputBus& bus_;
  core::events::ComputeHandles handles_;
  std::uint64_t pendingRequestId_{0};

  message_bus::Subscription<core::events::ComputeOutputResult> sub_;

  static std::atomic<std::uint64_t> request_counter_;
};

}  // namespace vkit::workflow
