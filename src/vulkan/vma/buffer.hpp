#pragma once

#include <vk_mem_alloc.h>

#include "util/scoped.hpp"
#include "vulkan/command_block.hpp"

namespace vkit::vulkan::vma {
struct RawBuffer {
  [[nodiscard]] auto mapped_span() const -> std::span<std::byte> {
    return std::span{static_cast<std::byte*>(mapped), size};
  }

  [[nodiscard]] auto device_address() const -> vk::DeviceAddress {
    return address;
  }

  auto operator==(RawBuffer const& rhs) const -> bool = default;

  VmaAllocator allocator{};
  VmaAllocation allocation{};
  vk::Buffer buffer;
  vk::DeviceSize size{};
  vk::DeviceAddress address{};
  void* mapped{};
};

struct BufferDeleter {
  void operator()(RawBuffer const& raw_buffer) const noexcept;
};

using Buffer = util::Scoped<RawBuffer, BufferDeleter>;

struct BufferCreateInfo {
  vk::Device device;
  VmaAllocator allocator;
  vk::BufferUsageFlags usage;
  std::uint32_t queue_family;
};

enum class BufferMemoryType : std::int8_t { kHost, kDevice };

[[nodiscard]] auto create_buffer(BufferCreateInfo const& create_info,
                                 BufferMemoryType memory_type,
                                 vk::DeviceSize size) -> Buffer;

using ByteSpans = std::span<std::span<std::byte const> const>;

[[nodiscard]] auto create_device_buffer(BufferCreateInfo const& create_info,
                                        CommandBlock command_block,
                                        ByteSpans const& byte_spans) -> Buffer;
};  // namespace vkit::vulkan::vma
