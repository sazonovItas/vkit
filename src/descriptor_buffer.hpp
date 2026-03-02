#pragma once

#include "resource_buffering.hpp"
#include "vk_mem_alloc.hpp"
#include "vku/buffers/allocated_buffer.hpp"

namespace lvk {
class DescriptorBuffer {
 public:
  explicit DescriptorBuffer(vma::Allocator allocator,
                            std::uint32_t queue_family,
                            vk::BufferUsageFlags usage);

  void write_at(std::size_t frame_index, std::span<std::byte const> bytes);

  [[nodiscard]] auto descriptor_info_at(std::size_t frame_index) const
      -> vk::DescriptorBufferInfo;

 private:
  void write_to(std::optional<vku::AllocatedBuffer>& out,
                std::span<std::byte const> bytes) const;

  vma::Allocator m_allocator_;
  std::uint32_t m_queue_family_{};
  vk::BufferUsageFlags m_usage_;
  Buffered<std::optional<vku::AllocatedBuffer>> m_buffers_;
};
};  // namespace lvk
