#pragma once

#include <vk_mem_alloc.h>

#include "resource_buffering.hpp"
#include "vulkan/vma/buffer.hpp"

namespace lvk {
class DescriptorBuffer {
 public:
  explicit DescriptorBuffer(VmaAllocator allocator, std::uint32_t queue_family,
                            vk::BufferUsageFlags usage);

  void write_at(std::size_t frame_index, std::span<std::byte const> bytes);

  [[nodiscard]] auto descriptor_info_at(std::size_t frame_index) const
      -> vk::DescriptorBufferInfo;

 private:
  struct Buffer {
    vkit::vulkan::vma::Buffer buffer;
    vk::DeviceSize size{};
  };

  void write_to(Buffer& out, std::span<std::byte const> bytes) const;

  VmaAllocator m_allocator_{};
  std::uint32_t m_queue_family_{};
  vk::BufferUsageFlags m_usage_;
  Buffered<Buffer> m_buffers_{};
};
};  // namespace lvk
