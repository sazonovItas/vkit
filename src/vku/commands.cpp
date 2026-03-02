#include "commands.hpp"

namespace vku {
template <std::size_t N>
auto allocate_command_buffers(vk::Device device, vk::CommandPool command_pool,
                              vk::CommandBufferLevel level)
    -> std::array<vk::CommandBuffer, N> {
  std::array<vk::CommandBuffer, N> command_buffers;
  const auto allocate_infos = std::array<vk::CommandBufferAllocateInfo, 1>{
      vk::CommandBufferAllocateInfo{command_pool, level, N},
  };
  const auto result =
      device.allocateCommandBuffers(allocate_infos, command_buffers.data());
  if (result != vk::Result::eSuccess) {
    throw std::runtime_error{"Failed to allocate command buffers"};
  }

  return command_buffers;
}

template <std::invocable<vk::CommandBuffer> F>
  requires std::is_void_v<std::invoke_result_t<F, vk::CommandBuffer>>
void execute_command(vk::Device device, vk::CommandPool command_pool,
                     vk::Queue queue, F &&f, vk::Fence fence) {
  const vk::CommandBuffer command_buffer =
      allocate_command_buffers<1>(device, command_pool)[0];

  command_buffer.begin({vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
  auto result = std::invoke(std::forward<F>(f), command_buffer);
  command_buffer.end();

  queue.submit2(
      vk::SubmitInfo2{
          {},
          {},
          command_buffer,
      },
      fence);

  return result;
}
};  // namespace vku
