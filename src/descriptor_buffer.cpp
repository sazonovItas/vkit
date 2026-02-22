#include "descriptor_buffer.hpp"

#include <array>
#include <cstdint>
#include <cstring>

namespace lvk {
DescriptorBuffer::DescriptorBuffer(VmaAllocator allocator,
                                   std::uint32_t const queue_family,
                                   vk::BufferUsageFlags const usage)
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
  auto ret = vk::DescriptorBufferInfo{};
  ret.setBuffer(buffer.buffer.get().buffer).setRange(buffer.size);
  return ret;
}

void DescriptorBuffer::write_to(Buffer& out,
                                std::span<std::byte const> bytes) const {
  static constexpr auto kBlankByteV = std::array{std::byte{}};
  if (bytes.empty()) {
    bytes = kBlankByteV;
  }

  out.size = bytes.size();
  if (out.buffer.get().size < bytes.size()) {
    auto const buffer_ci = vkit::vulkan::vma::BufferCreateInfo{
        .allocator = m_allocator_,
        .usage = m_usage_,
        .queue_family = m_queue_family_,
    };
    out.buffer = vkit::vulkan::vma::create_buffer(
        buffer_ci, vkit::vulkan::vma::BufferMemoryType::kHost, out.size);
  }

  std::memcpy(out.buffer.get().mapped, bytes.data(), bytes.size());
}
};  // namespace lvk
