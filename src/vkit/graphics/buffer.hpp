#pragma once

#include <vk_mem_alloc.hpp>

#include "vkit/graphics/allocation.hpp"

namespace vkit::graphics {

struct Buffer {
  vk::Buffer buffer;
  std::size_t size;

  [[nodiscard]] explicit operator vk::Buffer() const noexcept { return buffer; }

  [[nodiscard]] auto getCreateInfo(vk::Format format, std::size_t offset = 0,
                                   std::size_t range = vk::WholeSize)
      const noexcept -> vk::BufferViewCreateInfo {
    return {{}, buffer, format, offset, range};
  }
};

struct AllocatedBuffer : Buffer {
  vma::Allocator allocator;
  vma::Allocation allocation;

  AllocatedBuffer(vma::Allocator allocator,
                  const vk::BufferCreateInfo& createInfo,
                  const vma::AllocationCreateInfo& allocationCreateInfo =
                      vkit::graphics::allocation::kDeviceLocal)
      : allocator{allocator} {
    size = createInfo.size;
    std::tie(allocation, buffer) =
        allocator.createBuffer(createInfo, allocationCreateInfo);
  }

  AllocatedBuffer() = default;
  AllocatedBuffer(const AllocatedBuffer&) = delete;
  AllocatedBuffer& operator=(const AllocatedBuffer&) = delete;

  AllocatedBuffer(AllocatedBuffer&& src) noexcept
      : Buffer{static_cast<Buffer>(src)},
        allocator{src.allocator},
        allocation{std::exchange(src.allocation, nullptr)} {
    buffer = std::exchange(src.buffer, nullptr);
  }

  AllocatedBuffer& operator=(AllocatedBuffer&& src) noexcept {
    if (allocation) {
      allocator.destroyBuffer(buffer, allocation);
    }

    static_cast<Buffer&>(*this) = static_cast<Buffer>(src);
    allocator = src.allocator;
    allocation = std::exchange(src.allocation, nullptr);
    buffer = std::exchange(src.buffer, nullptr);
    return *this;
  }

  virtual ~AllocatedBuffer() {
    if (allocation) {
      allocator.destroyBuffer(buffer, allocation);
    }
  }
};

};  // namespace vkit::graphics
