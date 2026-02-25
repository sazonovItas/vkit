#include "util.hpp"

#include <chrono>
#include <print>

namespace vkit::vulkan::util {
void record_and_submit(vk::Device device, vk::Queue queue, vk::CommandPool cp,
                       std::function<void(vk::CommandBuffer)> &&record) {
  auto allocate_info = vk::CommandBufferAllocateInfo{}
                           .setCommandPool(cp)
                           .setCommandBufferCount(1)
                           .setLevel(vk::CommandBufferLevel::ePrimary);

  auto cb = std::move(device.allocateCommandBuffers(allocate_info).front());

  auto begin_info = vk::CommandBufferBeginInfo{}.setFlags(
      vk::CommandBufferUsageFlagBits::eOneTimeSubmit);

  cb.begin(begin_info);
  record(cb);
  cb.end();

  auto cb_info = vk::CommandBufferSubmitInfo{}.setCommandBuffer(cb);
  auto submit_info = vk::SubmitInfo2{}.setCommandBufferInfos(cb_info);
  auto fence = device.createFenceUnique({});

  queue.submit2(submit_info, *fence);

  static constexpr auto kTimeoutV = static_cast<std::uint64_t>(
      std::chrono::nanoseconds{std::chrono::seconds{30}}.count());
  auto const result = device.waitForFences(*fence, vk::True, kTimeoutV);
  if (result != vk::Result::eSuccess) {
    std::println(stderr, "failed to submit Command Buffer");
  }

  cb.reset();
}

CommandBlock::CommandBlock(vk::Device const device, vk::Queue const queue,
                           vk::CommandPool const command_pool)
    : device_(device), queue_(queue) {
  auto allocate_info = vk::CommandBufferAllocateInfo{};
  allocate_info.setCommandPool(command_pool)
      .setCommandBufferCount(1)
      .setLevel(vk::CommandBufferLevel::ePrimary);

  cb_ = std::move(device_.allocateCommandBuffersUnique(allocate_info).front());

  auto begin_info = vk::CommandBufferBeginInfo{};
  begin_info.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
  cb_->begin(begin_info);
}

void CommandBlock::submit_and_wait() {
  if (!cb_) {
    return;
  }

  cb_->end();

  auto submit_info = vk::SubmitInfo2KHR{};
  auto const command_buffer_info = vk::CommandBufferSubmitInfo(*cb_);
  submit_info.setCommandBufferInfos(command_buffer_info);
  auto fence = device_.createFenceUnique({});
  queue_.submit2(submit_info, *fence);

  static constexpr auto kTimeoutV = static_cast<std::uint64_t>(
      std::chrono::nanoseconds{std::chrono::seconds{30}}.count());
  auto const result = device_.waitForFences(*fence, vk::True, kTimeoutV);
  if (result != vk::Result::eSuccess) {
    std::println(stderr, "failed to submit Command Buffer");
  }

  cb_.reset();
}
};  // namespace vkit::vulkan::util
