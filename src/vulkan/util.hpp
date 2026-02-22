#pragma once

namespace vkit::vulkan::util {
template <typename T, typename E>
auto contains(T flags, E bit) -> bool {
  return (flags & bit) == bit;
}

static void record_and_submit(vk::Device device, vk::Queue queue,
                              vk::CommandPool cp,
                              std::function<void(vk::CommandBuffer)>&& record);
};  // namespace vkit::vulkan::util
