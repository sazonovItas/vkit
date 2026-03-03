#include "descriptor_buffer.hpp"

#include <array>
#include <cstdint>
#include <cstring>

#include "vku/buffers/allocated_buffer.hpp"
#include "vku/constants.hpp"

namespace lvk {
DescriptorBuffer::DescriptorBuffer(vma::Allocator allocator,
                                   std::uint32_t queue_family,
                                   vk::BufferUsageFlags usage)
    : allocator_{allocator}, queueFamily_{queue_family}, usage_{usage} {
  for (auto& buffer : buffers_) {
    writeTo(buffer, {});
  }
}

void DescriptorBuffer::write_at(std::size_t const frame_index,
                                std::span<std::byte const> bytes) {
  writeTo(buffers_.at(frame_index), bytes);
}

auto DescriptorBuffer::descriptorInfoAt(std::size_t const frame_index) const
    -> vk::DescriptorBufferInfo {
  auto const& buffer = buffers_.at(frame_index);
  return vk::DescriptorBufferInfo{}
      .setBuffer(buffer.buffer)
      .setRange(buffer.size);
}

void DescriptorBuffer::writeTo(vku::AllocatedBuffer& out,
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
};  // namespace lvk
