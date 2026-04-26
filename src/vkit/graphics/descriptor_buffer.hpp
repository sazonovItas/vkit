
#pragma once

#include <vk_mem_alloc.hpp>

#include "vkit/dataformat/vertex_format.hpp"
#include "vkit/graphics/buffer.hpp"

namespace vkit::graphics {

class DescriptorBuffer {
 public:
  explicit DescriptorBuffer(vma::Allocator allocator,
                            std::uint32_t queue_family,
                            dataformat::BufferUsageFlags usage)
      : allocator_{allocator},
        queueFamily_{queue_family},
        usage_{usage},
        buffer_{
            allocator,
            vk::BufferCreateInfo{}.setUsage(usage).setSize(0),
            allocation::kHostWrite,
        } {}

  void writeAt(std::span<const std::byte> bytes) { writeTo(buffer_, bytes); }

  [[nodiscard]] auto descriptorInfo() const -> vk::DescriptorBufferInfo {
    return vk::DescriptorBufferInfo{}
        .setBuffer(buffer_.buffer)
        .setRange(buffer_.size);
  }

 private:
  void writeTo(AllocatedBuffer& out, std::span<std::byte const> bytes) const {
    static constexpr auto kBlankByteV = std::array{std::byte{}};
    if (bytes.empty()) {
      bytes = kBlankByteV;
    }

    if (out.size < bytes.size()) {
      auto buffer_ci =
          vk::BufferCreateInfo{}.setSize(bytes.size_bytes()).setUsage(usage_);
      out = {allocator_, buffer_ci, allocation::kHostWrite};
    }

    allocator_.copyMemoryToAllocation(bytes.data(), out.allocation, {},
                                      bytes.size_bytes());
  }

  vma::Allocator allocator_;
  std::uint32_t queueFamily_{};
  vk::BufferUsageFlags usage_;
  AllocatedBuffer buffer_;
};

};  // namespace vkit::graphics
