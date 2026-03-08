#pragma once

#include "vk_mem_alloc.hpp"
#include "vku/buffers/allocated_buffer.hpp"

namespace lvk {
template <std::uint32_t N = 1>
class DescriptorBuffer {
 public:
  explicit DescriptorBuffer(vma::Allocator allocator,
                            std::uint32_t queue_family,
                            vk::BufferUsageFlags usage)
      : allocator_{allocator}, queueFamily_{queue_family}, usage_{usage} {
    for (auto& buffer : buffers_) {
      writeTo(buffer, {});
    }
  }

  void writeAt(std::size_t frame_index, std::span<const std::byte> bytes) {
    writeTo(buffers_.at(frame_index), bytes);
  }

  [[nodiscard]] auto descriptorInfoAt(std::size_t idx) const
      -> vk::DescriptorBufferInfo {
    auto const& buffer = buffers_.at(idx);
    return vk::DescriptorBufferInfo{}
        .setBuffer(buffer.buffer)
        .setRange(buffer.size);
  }

 private:
  void writeTo(vku::AllocatedBuffer& out,
               std::span<std::byte const> bytes) const {
    static constexpr auto kBlankByteV = std::array{std::byte{}};
    if (bytes.empty()) {
      bytes = kBlankByteV;
    }

    if (out.size < bytes.size()) {
      auto buffer_ci =
          vk::BufferCreateInfo{}.setSize(bytes.size_bytes()).setUsage(usage_);
      out = {allocator_, buffer_ci, vku::allocation::kHostWrite};
    }

    allocator_.copyMemoryToAllocation(bytes.data(), out.allocation, {},
                                      bytes.size_bytes());
  }

  vma::Allocator allocator_;
  std::uint32_t queueFamily_{};
  vk::BufferUsageFlags usage_;
  std::array<vku::AllocatedBuffer, N> buffers_;
};
};  // namespace lvk
