#pragma once

#include <concepts>
#include <type_traits>

namespace vku {
template <std::size_t N>
[[nodiscard]] auto allocate_command_buffers(vk::Device device,
                                            vk::CommandPool command_pool,
                                            vk::CommandBufferLevel level = {})
    -> std::array<vk::CommandBuffer, N>;

template <std::invocable<vk::CommandBuffer> F>
  requires std::is_void_v<std::invoke_result_t<F, vk::CommandBuffer>>
void execute_command(vk::Device device, vk::CommandPool command_pool,
                     vk::Queue queue, F &&f, vk::Fence fence = {});
};  // namespace vku
