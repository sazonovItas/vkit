#pragma once

#include <vk_mem_alloc.h>

#include "util/scoped.hpp"

namespace vkit::vulkan::vma {
struct AllocatorDeleter {
  void operator()(VmaAllocator allocator) const noexcept;
};

using Allocator = util::Scoped<VmaAllocator, AllocatorDeleter>;

auto create_allocator(vk::Instance instance, vk::PhysicalDevice physical_device,
                      vk::Device device) -> Allocator;
};  // namespace vkit::vulkan::vma
