#include "command_block.hpp"

#include <chrono>
#include <print>

namespace vkit::vulkan {
CommandBlock::CommandBlock(vk::Device const device, vk::Queue const queue,
                           vk::CommandPool const command_pool)
    : m_device_(device), m_queue_(queue) {
  auto allocate_info = vk::CommandBufferAllocateInfo{};
  allocate_info.setCommandPool(command_pool)
      .setCommandBufferCount(1)
      .setLevel(vk::CommandBufferLevel::ePrimary);

  auto command_buffers = m_device_.allocateCommandBuffersUnique(allocate_info);
  m_cb_ = std::move(command_buffers.front());

  auto begin_info = vk::CommandBufferBeginInfo{};
  begin_info.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
  m_cb_->begin(begin_info);
}

void CommandBlock::submit_and_wait() {
  if (!m_cb_) {
    return;
  }

  m_cb_->end();

  auto submit_info = vk::SubmitInfo2KHR{};
  auto const command_buffer_info = vk::CommandBufferSubmitInfo(*m_cb_);
  submit_info.setCommandBufferInfos(command_buffer_info);
  auto fence = m_device_.createFenceUnique({});
  m_queue_.submit2(submit_info, *fence);

  static constexpr auto kTimeoutV = static_cast<std::uint64_t>(
      std::chrono::nanoseconds{std::chrono::seconds{30}}.count());
  auto const result = m_device_.waitForFences(*fence, vk::True, kTimeoutV);
  if (result != vk::Result::eSuccess) {
    std::println(stderr, "failed to submit Command Buffer");
  }

  m_cb_.reset();
}
};  // namespace vkit::vulkan
