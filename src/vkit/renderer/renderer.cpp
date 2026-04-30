#include "vkit/renderer/renderer.hpp"

#include <limits>

#include "vkit/graphics/util.hpp"

namespace vkit::renderer {

Renderer::Renderer(vk::Device device, vk::Queue graphicsQueue,
                   std::uint32_t queueFamilyIndex, std::uint32_t framesInFlight)
    : device_{device},
      graphicsQueue_{graphicsQueue},
      framesInFlight_{framesInFlight},
      waiter_(device) {
  auto pool_info = vk::CommandPoolCreateInfo{};
  pool_info.setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer)
      .setQueueFamilyIndex(queueFamilyIndex);
  commandPool_ = device_.createCommandPoolUnique(pool_info);

  frames_.resize(framesInFlight_);
  auto alloc_info = vk::CommandBufferAllocateInfo{};
  alloc_info.setCommandPool(commandPool_.get())
      .setLevel(vk::CommandBufferLevel::ePrimary)
      .setCommandBufferCount(framesInFlight_);
  auto command_buffers = device_.allocateCommandBuffersUnique(alloc_info);

  auto semaphore_info = vk::SemaphoreCreateInfo{};
  auto fence_info = vk::FenceCreateInfo{vk::FenceCreateFlagBits::eSignaled};

  for (std::uint32_t i = 0; i < framesInFlight_; i++) {
    frames_[i].cb = std::move(command_buffers[i]);
    frames_[i].renderFinished = device_.createSemaphoreUnique(semaphore_info);
    frames_[i].inFlightFence = device_.createFenceUnique(fence_info);
  }
}

void Renderer::beginFrame(std::uint32_t frameIndex) {
  assert(frameIndex < framesInFlight_ && "Renderer: frameIndex out of bounds");
  assert(!isFrameStarted_ &&
         "Renderer: Cannot call beginFrame() while a frame is already in "
         "progress");

  Frame& frame = frames_[frameIndex];

  graphics::util::requireSuccess(
      device_.waitForFences(frame.inFlightFence.get(), vk::True,
                            std::numeric_limits<std::uint64_t>::max()),
      "Failed to wait for in-flight fence");

  isFrameStarted_ = true;
}

auto Renderer::submit(std::uint32_t frameIndex, const RenderTask& task,
                      vk::Semaphore waitSemaphore,
                      vk::PipelineStageFlags wait_stage) -> RenderResult {
  assert(frameIndex < framesInFlight_ && "Renderer: frameIndex out of bounds!");
  assert(isFrameStarted_ && "Must call beginFrame() before calling submit()!");

  Frame& frame = frames_[frameIndex];

  if (task.commands.empty()) {
    isFrameStarted_ = false;
    return {
        .renderFinishedSemaphore = nullptr,
    };
  }

  device_.resetFences(frame.inFlightFence.get());

  auto cb = frame.cb.get();
  cb.reset();

  auto begin_info = vk::CommandBufferBeginInfo{};
  cb.begin(begin_info);

  for (const auto& cmd : task.commands) {
    cmd->record(cb);
  }

  cb.end();

  auto submit_info = vk::SubmitInfo{};
  submit_info.setCommandBuffers(cb).setSignalSemaphores(
      frame.renderFinished.get());

  if (waitSemaphore) {
    submit_info.setWaitSemaphores(waitSemaphore)
        .setWaitDstStageMask(wait_stage);
  }

  graphicsQueue_.submit(submit_info, frame.inFlightFence.get());

  isFrameStarted_ = false;

  return RenderResult{
      .renderFinishedSemaphore = frame.renderFinished.get(),
  };
}

};  // namespace vkit::renderer
