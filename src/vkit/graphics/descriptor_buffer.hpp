#pragma once

#include <span>
#include <vk_mem_alloc.hpp>

#include "vkit/dataformat/vertex_format.hpp"
#include "vkit/graphics/buffer.hpp"

namespace vkit::graphics {

class DescriptorBuffer {
 public:
  explicit DescriptorBuffer(vma::Allocator allocator,
                            dataformat::BufferUsageFlags usage,
                            std::uint32_t bufferSize = 256)
      : allocator_{allocator},
        usage_{usage},
        buffer_{
            allocator,
            vk::BufferCreateInfo{}.setUsage(usage).setSize(bufferSize),
            allocation::kHostWrite,
        } {}

  [[nodiscard]] auto writeAt(std::span<const std::byte> bytes) -> bool {
    return writeTo(buffer_, bytes);
  }

  [[nodiscard]] auto descriptorInfo() const -> vk::DescriptorBufferInfo {
    return vk::DescriptorBufferInfo{}
        .setBuffer(buffer_.buffer)
        .setRange(buffer_.size);
  }

  [[nodiscard]] auto getSize() const -> std::size_t { return buffer_.size; }

 private:
  [[nodiscard]] auto writeTo(AllocatedBuffer& out,
                             std::span<std::byte const> bytes) -> bool {
    if (bytes.empty()) return false;

    bool reallocated = false;

    if (out.size < bytes.size_bytes()) {
      size_t new_size = bytes.size_bytes();
      if (out.size > 0) {
        new_size = std::max(new_size, static_cast<size_t>(out.size * 1.5));
      }

      auto buffer_ci =
          vk::BufferCreateInfo{}.setSize(new_size).setUsage(usage_);
      out = AllocatedBuffer{allocator_, buffer_ci, allocation::kHostWrite};
      reallocated = true;
    }

    allocator_.copyMemoryToAllocation(bytes.data(), out.allocation, 0,
                                      bytes.size_bytes());

    return reallocated;
  }

  vma::Allocator allocator_;
  vk::BufferUsageFlags usage_;
  AllocatedBuffer buffer_;
};

};  // namespace vkit::graphics
