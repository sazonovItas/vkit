#include "vkit/compute/async_compute.hpp"

namespace vkit::compute {

AsyncCompute::AsyncCompute(vk::Device device, vk::Queue computeQueue,
                           std::uint32_t queueFamilyIndex)
    : device_{device}, queue_{computeQueue} {
  auto pool_info = vk::CommandPoolCreateInfo{}
                       .setQueueFamilyIndex(queueFamilyIndex)
                       .setFlags(vk::CommandPoolCreateFlagBits::eTransient);

  commandPool_ = device_.createCommandPoolUnique(pool_info);
}

auto AsyncCompute::submit(const ComputeTask& task, vk::Fence computeFence)
    -> ComputeResult {
  auto alloc_info = vk::CommandBufferAllocateInfo{}
                        .setCommandPool(*commandPool_)
                        .setLevel(vk::CommandBufferLevel::ePrimary)
                        .setCommandBufferCount(1);

  auto cb = std::move(device_.allocateCommandBuffersUnique(alloc_info)[0]);

  cb->begin(vk::CommandBufferBeginInfo{}.setFlags(
      vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

  for (const auto& cmd : task.commands) {
    cmd->record(*cb);
  }

  cb->end();

  auto finished_semaphore = device_.createSemaphoreUnique({});

  auto submit_info =
      vk::SubmitInfo{}.setCommandBuffers(*cb).setSignalSemaphores(
          *finished_semaphore);

  queue_.submit(submit_info, computeFence);

  return ComputeResult{
      .finishedSemaphore = std::move(finished_semaphore),
  };
}

};  // namespace vkit::compute
