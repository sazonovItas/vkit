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

  [[nodiscard]] auto descriptorInfoAt(std::size_t frame_index) const
      -> vk::DescriptorBufferInfo;

 private:
  void writeTo(vku::AllocatedBuffer& out,
               std::span<std::byte const> bytes) const;

  vma::Allocator allocator_;
  std::uint32_t queueFamily_{};
  vk::BufferUsageFlags usage_;
  Buffered<vku::AllocatedBuffer> buffers_;
};
};  // namespace lvk
