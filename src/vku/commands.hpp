#pragma once

#include <chrono>
#include <concepts>
#include <type_traits>

namespace vku {
using namespace std::chrono_literals;

template <std::size_t N = 1>
[[nodiscard]] auto allocateCommandBuffers(vk::Device device,
                                          vk::CommandPool command_pool,
                                          vk::CommandBufferLevel level = {})
    -> std::array<vk::CommandBuffer, N> {
  std::array<vk::CommandBuffer, N> command_buffers;
  vk::CommandBufferAllocateInfo allocate_info{command_pool, level,
                                              static_cast<std::uint32_t>(N)};
  device.allocateCommandBuffers(&allocate_info, command_buffers.data());
  return command_buffers;
}
template <std::size_t N = 1>
[[nodiscard]] auto allocateUniqueCommandBuffers(
    vk::Device device, vk::CommandPool command_pool,
    vk::CommandBufferLevel level = {})
    -> std::array<vk::UniqueCommandBuffer, N> {
  vk::CommandBufferAllocateInfo allocate_info{command_pool, level,
                                              static_cast<uint32_t>(N)};

  auto cbs = device.allocateCommandBuffersUnique(allocate_info);

  std::array<vk::UniqueCommandBuffer, N> command_buffers{};
  for (std::size_t i = 0; i < N; i++) {
    command_buffers[i] = std::move(cbs[i]);
  }

  return command_buffers;
}

template <std::invocable<vk::CommandBuffer> F>
  requires std::is_void_v<std::invoke_result_t<F, vk::CommandBuffer>>
void executeCommand(vk::Device device, vk::CommandPool command_pool,
                    vk::Queue queue, F&& f, vk::Fence fence) {
  const vk::CommandBuffer command_buffer =
      allocateCommandBuffers(device, command_pool)[0];

  command_buffer.begin({vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
  std::invoke(std::forward<F>(f), command_buffer);
  command_buffer.end();

  queue.submit2(
      vk::SubmitInfo2{
          {},
          {},
          vk::CommandBufferSubmitInfo{}.setCommandBuffer(command_buffer),
      },
      fence);
}

template <std::invocable<vk::CommandBuffer> F>
  requires std::is_void_v<std::invoke_result_t<F, vk::CommandBuffer>>
void executeCommandAndWait(vk::Device device, vk::CommandPool command_pool,
                           vk::Queue queue, F&& f) {
  const auto command_buffer =
      std::move(allocateUniqueCommandBuffers(device, command_pool)[0]);

  command_buffer->begin({vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
  std::invoke(std::forward<F>(f), *command_buffer);
  command_buffer->end();

  auto fence = device.createFenceUnique({});

  queue.submit2(
      vk::SubmitInfo2{
          {},
          {},
          vk::CommandBufferSubmitInfo{}.setCommandBuffer(*command_buffer),
      },
      *fence);

  static constexpr auto kTimeoutV =
      static_cast<std::uint64_t>(std::chrono::nanoseconds{60s}.count());
  auto result = device.waitForFences(*fence, vk::True, kTimeoutV);
  if (result != vk::Result::eSuccess) {
    throw std::runtime_error{"failed to submit command buffer"};
  }
}
};  // namespace vku
