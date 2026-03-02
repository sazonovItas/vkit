#pragma once

#include <concepts>
#include <cstdint>
#include <type_traits>

#include "vk_mem_alloc.hpp"
#include "vku/buffers/allocated_buffer.hpp"
#include "vku/constants.hpp"
#include "vku/queue.hpp"

namespace vku {
class MappedBuffer : public AllocatedBuffer {
 public:
  void* data;

  MappedBuffer(vma::Allocator allocator,
               const vk::BufferCreateInfo& create_info,
               const vma::AllocationCreateInfo& allocation_create_info =
                   allocation::kHostWrite)
      : AllocatedBuffer{allocator, create_info, allocation_create_info},
        data{allocator.mapMemory(allocation)} {}

  template <typename T>
    requires(!std::same_as<T, std::from_range_t> &&
             std::is_trivially_copyable_v<T>)
  MappedBuffer(vma::Allocator allocator, const T& value,
               vk::BufferUsageFlags usage,
               const vma::AllocationCreateInfo& allocation_create_info =
                   allocation::kHostWrite)
      : MappedBuffer{allocator,
                     vk::BufferCreateInfo{
                         {},
                         sizeof(T),
                         usage,
                     },
                     allocation_create_info} {
    *static_cast<T*>(data) = value;
  }

  template <typename T>
    requires(!std::same_as<T, std::from_range_t> &&
             std::is_trivially_copyable_v<T>)
  MappedBuffer(vma::Allocator allocator, const T& value,
               vk::BufferUsageFlags usage,
               vk::ArrayProxy<const std::uint32_t> queue_family_indices,
               const vma::AllocationCreateInfo& allocation_create_info =
                   allocation::kHostWrite)
      : MappedBuffer{allocator,
                     vk::BufferCreateInfo{
                         {},
                         sizeof(T),
                         usage,
                         get_sharing_mode(queue_family_indices),
                         queue_family_indices,
                     },
                     allocation_create_info} {
    *static_cast<T*>(data) = value;
  }

  template <std::ranges::input_range R>
    requires(std::ranges::sized_range<R> &&
             std::is_trivially_copyable_v<std::ranges::range_value_t<R>>)
  MappedBuffer(vma::Allocator allocator, std::from_range_t, R&& r,
               vk::BufferUsageFlags usage,
               const vma::AllocationCreateInfo& allocation_create_info =
                   allocation::kHostWrite)
      : MappedBuffer{allocator,
                     vk::BufferCreateInfo{
                         {},
                         r.size() * sizeof(std::ranges::range_value_t<R>),
                         usage,
                     },
                     allocation_create_info} {
    std::ranges::copy(std::forward<R>(r),
                      static_cast<std::ranges::range_value_t<R>*>(data));
  }

  template <std::ranges::input_range R>
    requires(std::ranges::sized_range<R> &&
             std::is_trivially_copyable_v<std::ranges::range_value_t<R>>)
  MappedBuffer(vma::Allocator allocator, std::from_range_t, R&& r,
               vk::BufferUsageFlags usage,
               vk::ArrayProxy<const std::uint32_t> queue_family_indices,
               const vma::AllocationCreateInfo& allocation_create_info =
                   allocation::kHostWrite)
      : MappedBuffer{allocator,
                     vk::BufferCreateInfo{
                         {},
                         r.size() * sizeof(std::ranges::range_value_t<R>),
                         usage,
                         get_sharing_mode(queue_family_indices),
                         queue_family_indices,
                     },
                     allocation_create_info} {
    std::ranges::copy(std::forward<R>(r),
                      static_cast<std::ranges::range_value_t<R>*>(data));
  }

  MappedBuffer(const MappedBuffer&) = delete;

  MappedBuffer(MappedBuffer&& src) noexcept = default;

  MappedBuffer& operator=(const MappedBuffer&) = delete;

  MappedBuffer& operator=(MappedBuffer&& src) noexcept {
    if (allocation) {
      allocator.unmapMemory(allocation);
    }

    static_cast<AllocatedBuffer&>(*this) =
        std::move(static_cast<AllocatedBuffer&>(src));
    data = src.data;
    return *this;
  }

  ~MappedBuffer() override {
    if (allocation) {
      allocator.unmapMemory(allocation);
    }
  }

  template <typename T>
  [[nodiscard]] auto as_range(vk::DeviceSize byte_offset = 0) const
      -> std::span<const T> {
    assert(byte_offset <= size && "Out of bound: byteOffset > size");
    return {reinterpret_cast<const T*>(static_cast<const char*>(data) +
                                       byte_offset),
            (size - byte_offset) / sizeof(T)};
  }

  template <typename T>
  [[nodiscard]] auto as_range(vk::DeviceSize byte_offset = 0) -> std::span<T> {
    assert(byte_offset <= size && "Out of bound: byteOffset > size");
    return {reinterpret_cast<T*>(static_cast<char*>(data) + byte_offset),
            (size - byte_offset) / sizeof(T)};
  }

  template <typename T>
  [[nodiscard]] auto as_value(vk::DeviceSize byte_offset = 0) const
      -> const T& {
    assert(byte_offset + sizeof(T) <= size &&
           "Out of bound: byteOffset + sizeof(T) > size");
    return *reinterpret_cast<const T*>(static_cast<const char*>(data) +
                                       byte_offset);
  }

  template <typename T>
  [[nodiscard]] auto as_value(vk::DeviceSize byte_offset = 0) -> T& {
    assert(byte_offset + sizeof(T) <= size &&
           "Out of bound: byteOffset + sizeof(T) > size");
    return *reinterpret_cast<T*>(static_cast<char*>(data) + byte_offset);
  }

  [[nodiscard]] auto unmap() && noexcept {
    allocator.unmapMemory(allocation);
    return static_cast<AllocatedBuffer>(std::move(*this));
  }

 private:
  explicit MappedBuffer(AllocatedBuffer&& allocated_buffer)
      : AllocatedBuffer{std::move(allocated_buffer)},
        data{allocator.mapMemory(allocation)} {}
};
};  // namespace vku
