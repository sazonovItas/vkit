#pragma once

namespace lvk {
constexpr auto kVkVersionV = VK_MAKE_VERSION(1, 3, 0);

struct Gpu {
  vk::PhysicalDevice device;
  vk::PhysicalDeviceProperties properties;
  vk::PhysicalDeviceFeatures features;
  std::uint32_t queue_family;
};

[[nodiscard]] auto get_suitable_gpu(vk::Instance instance,
                                    vk::SurfaceKHR surface) -> Gpu;
};  // namespace lvk
