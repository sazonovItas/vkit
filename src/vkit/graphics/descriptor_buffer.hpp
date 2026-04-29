#pragma once

#include <vk_mem_alloc.hpp>

#include "vkit/dataformat/vertex_format.hpp"
#include "vkit/graphics/buffer.hpp"

namespace vkit::graphics {

class DescriptorBuffer {
 public:
  static constexpr std::uint32_t kBufferDefaultSize = 256;

  explicit DescriptorBuffer(vma::Allocator allocator,
                            dataformat::BufferUsageFlags usage)
      : allocator_{allocator},
        usage_{usage},
        buffer_{
            allocator,
            vk::BufferCreateInfo{}.setUsage(usage).setSize(kBufferDefaultSize),
            allocation::kHostWrite,
        } {}

  void writeAt(std::span<const std::byte> bytes) { writeTo(buffer_, bytes); }

  [[nodiscard]] auto descriptorInfo() const -> vk::DescriptorBufferInfo {
    return vk::DescriptorBufferInfo{}
        .setBuffer(buffer_.buffer)
        .setRange(buffer_.size);
  }

 private:
  void writeTo(AllocatedBuffer& out, std::span<std::byte const> bytes) {
    if (bytes.empty()) return;

    if (out.size < bytes.size_bytes()) {
      size_t new_size = bytes.size_bytes();
      if (out.size > 0) {
        new_size = std::max(new_size, static_cast<size_t>(out.size * 1.5));
      }

      auto buffer_ci =
          vk::BufferCreateInfo{}.setSize(new_size).setUsage(usage_);

      out = AllocatedBuffer{allocator_, buffer_ci, allocation::kHostWrite};
    }

    allocator_.copyMemoryToAllocation(bytes.data(), out.allocation, 0,
                                      bytes.size_bytes());
  }

  vma::Allocator allocator_;
  vk::BufferUsageFlags usage_;
  AllocatedBuffer buffer_;
};

};  // namespace vkit::graphics
