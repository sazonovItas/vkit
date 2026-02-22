#include "allocator.hpp"

namespace vkit::vulkan::vma {
void AllocatorDeleter::operator()(VmaAllocator allocator) const noexcept {
  vmaDestroyAllocator(allocator);
}

auto create_allocator(vk::Instance const instance,
                      vk::PhysicalDevice const physical_device,
                      vk::Device const device) -> Allocator {
  auto const& dispatcher = VULKAN_HPP_DEFAULT_DISPATCHER;

  auto vma_vk_funcs = VmaVulkanFunctions{};
  vma_vk_funcs.vkGetInstanceProcAddr = dispatcher.vkGetInstanceProcAddr;
  vma_vk_funcs.vkGetDeviceProcAddr = dispatcher.vkGetDeviceProcAddr;

  auto allocator_ci = VmaAllocatorCreateInfo{};
  allocator_ci.physicalDevice = physical_device;
  allocator_ci.device = device;
  allocator_ci.pVulkanFunctions = &vma_vk_funcs;
  allocator_ci.instance = instance;
  allocator_ci.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;

  VmaAllocator ret{};
  auto const result = vmaCreateAllocator(&allocator_ci, &ret);
  if (result == VK_SUCCESS) {
    return ret;
  }

  throw std::runtime_error{"failed to create Vulkan Memory Allocator"};
}
};  // namespace vkit::vulkan::vma
