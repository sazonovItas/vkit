#include "command_block.hpp"

#include <chrono>
#include <print>

#include "vulkan/vulkan.hpp"

namespace lvk {
CommandBlock::CommandBlock(vk::Device const device, vk::Queue const queue,
                           vk::CommandPool const command_pool)
    : m_device_(device), m_queue_(queue) {
  auto allocate_info = vk::CommandBufferAllocateInfo{};
  allocate_info.setCommandPool(command_pool)
      .setCommandBufferCount(1)
      .setLevel(vk::CommandBufferLevel::ePrimary);

  auto command_buffers = m_device_.allocateCommandBuffersUnique(allocate_info);
  m_command_buffer_ = std::move(command_buffers.front());

  auto begin_info = vk::CommandBufferBeginInfo{};
  begin_info.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
  m_command_buffer_->begin(begin_info);
}

void CommandBlock::submit_and_wait() {
  if (!m_command_buffer_) {
    return;
  }

  m_command_buffer_->end();

  auto submit_info = vk::SubmitInfo2KHR{};
  auto const command_buffer_info =
      vk::CommandBufferSubmitInfo(*m_command_buffer_);
  submit_info.setCommandBufferInfos(command_buffer_info);
  auto fence = m_device_.createFenceUnique({});
  m_queue_.submit2(submit_info, *fence);

  static constexpr auto kTimeoutV = static_cast<std::uint64_t>(
      std::chrono::nanoseconds{std::chrono::seconds{30}}.count());
  auto const result = m_device_.waitForFences(*fence, vk::True, kTimeoutV);
  if (result != vk::Result::eSuccess) {
    std::println(stderr, "failed to submit Command Buffer");
  }

  m_command_buffer_.reset();
}
};  // namespace lvk
