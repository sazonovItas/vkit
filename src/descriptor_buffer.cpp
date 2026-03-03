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
    : m_allocator_{allocator}, m_queue_family_{queue_family}, m_usage_{usage} {
  for (auto& buffer : m_buffers_) {
    write_to(buffer, {});
  }
}

void DescriptorBuffer::write_at(std::size_t const frame_index,
                                std::span<std::byte const> bytes) {
  write_to(m_buffers_.at(frame_index), bytes);
}

auto DescriptorBuffer::descriptor_info_at(std::size_t const frame_index) const
    -> vk::DescriptorBufferInfo {
  auto const& buffer = m_buffers_.at(frame_index);
  return vk::DescriptorBufferInfo{}
      .setBuffer(buffer->buffer)
      .setRange(buffer->size);
}

void DescriptorBuffer::write_to(std::optional<vku::AllocatedBuffer>& out,
                                std::span<std::byte const> bytes) const {
  static constexpr auto kBlankByteV = std::array{std::byte{}};
  if (bytes.empty()) {
    bytes = kBlankByteV;
  }

  if (!out.has_value() || out->size < bytes.size()) {
    auto buffer_ci =
        vk::BufferCreateInfo{}.setSize(bytes.size_bytes()).setUsage(m_usage_);
    out.emplace(m_allocator_, buffer_ci, vku::allocation::kHostWrite);
  }

  m_allocator_.copyMemoryToAllocation(bytes.data(), out->allocation, {},
                                      bytes.size_bytes());
}
};  // namespace lvk
