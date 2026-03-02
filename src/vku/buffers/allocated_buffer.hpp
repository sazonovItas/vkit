#pragma once

#include <utility>

#include "buffer.hpp"
#include "vk_mem_alloc.hpp"

namespace vku {
struct AllocatedBuffer : Buffer {
  vma::Allocator allocator;
  vma::Allocation allocation;

  AllocatedBuffer(vma::Allocator allocator,
                  const vk::BufferCreateInfo& create_info,
                  const vma::AllocationCreateInfo& allocation_create_info =
                      {{}, vma::MemoryUsage::eAutoPreferDevice})
      : allocator{allocator} {
    std::tie(allocation, buffer) =
        allocator.createBuffer(create_info, allocation_create_info);
  }

  AllocatedBuffer(const AllocatedBuffer&) = delete;

  AllocatedBuffer(AllocatedBuffer&& src) noexcept
      : Buffer{static_cast<Buffer>(src)},
        allocator{src.allocator},
        allocation{std::exchange(src.allocation, nullptr)} {}

  AllocatedBuffer& operator=(const AllocatedBuffer&) = delete;

  AllocatedBuffer& operator=(AllocatedBuffer&& src) noexcept {
    if (allocation) {
      allocator.destroyBuffer(buffer, allocation);
    }

    static_cast<Buffer&>(*this) = static_cast<Buffer>(src);
    allocator = src.allocator;
    buffer = std::exchange(src.buffer, nullptr);
    allocation = std::exchange(src.allocation, nullptr);
    return *this;
  }

  virtual ~AllocatedBuffer() {
    if (allocation) {
      allocator.destroyBuffer(buffer, allocation);
    }
  }
};
};  // namespace vku
