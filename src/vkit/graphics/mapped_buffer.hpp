#pragma once

#include "vkit/graphics/buffer.hpp"
#include "vkit/graphics/util.hpp"

namespace vkit::graphics {

class MappedBuffer : public AllocatedBuffer {
 public:
  void* data;

  MappedBuffer(vma::Allocator allocator, const vk::BufferCreateInfo& createInfo,
               const vma::AllocationCreateInfo& allocationCreateInfo =
                   allocation::kHostWrite)
      : AllocatedBuffer{allocator, createInfo, allocationCreateInfo},
        data{allocator.mapMemory(allocation)} {}

  template <typename T>
    requires(!std::same_as<T, std::from_range_t> &&
             std::is_trivially_copyable_v<T>)
  MappedBuffer(vma::Allocator allocator, const T& value,
               vk::Flags<vk::BufferUsageFlagBits> usage,
               const vma::AllocationCreateInfo& allocationCreateInfo =
                   allocation::kHostWrite)
      : MappedBuffer{allocator,
                     vk::BufferCreateInfo{
                         {},
                         sizeof(T),
                         usage,
                     },
                     allocationCreateInfo} {
    *static_cast<T*>(data) = value;
  }

  template <typename T>
    requires(!std::same_as<T, std::from_range_t> &&
             std::is_trivially_copyable_v<T>)
  MappedBuffer(vma::Allocator allocator, const T& value,
               vk::Flags<vk::BufferUsageFlagBits> usage,
               vk::ArrayProxy<const std::uint32_t> queueFamilyIndices,
               const vma::AllocationCreateInfo& allocationCreateInfo =
                   allocation::kHostWrite)
      : MappedBuffer{allocator,
                     vk::BufferCreateInfo{
                         {},
                         sizeof(T),
                         usage,
                         util::getSharingMode(queueFamilyIndices),
                         queueFamilyIndices,
                     },
                     allocationCreateInfo} {
    *static_cast<T*>(data) = value;
  }

  template <std::ranges::input_range R>
    requires(std::ranges::sized_range<R> &&
             std::is_trivially_copyable_v<std::ranges::range_value_t<R>>)
  MappedBuffer(vma::Allocator allocator, std::from_range_t, R&& r,
               vk::Flags<vk::BufferUsageFlagBits> usage,
               const vma::AllocationCreateInfo& allocationCreateInfo =
                   allocation::kHostWrite)
      : MappedBuffer{allocator,
                     vk::BufferCreateInfo{
                         {},
                         r.size() * sizeof(std::ranges::range_value_t<R>),
                         usage,
                     },
                     allocationCreateInfo} {
    std::ranges::copy(std::forward<R>(r),
                      static_cast<std::ranges::range_value_t<R>*>(data));
  }

  template <std::ranges::input_range R>
    requires(std::ranges::sized_range<R> &&
             std::is_trivially_copyable_v<std::ranges::range_value_t<R>>)
  MappedBuffer(vma::Allocator allocator, std::from_range_t, R&& r,
               vk::Flags<vk::BufferUsageFlagBits> usage,
               vk::ArrayProxy<const std::uint32_t> queueFamilyIndices,
               const vma::AllocationCreateInfo& allocationCreateInfo =
                   allocation::kHostWrite)
      : MappedBuffer{allocator,
                     vk::BufferCreateInfo{
                         {},
                         r.size() * sizeof(std::ranges::range_value_t<R>),
                         usage,
                         util::getSharingMode(queueFamilyIndices),
                         queueFamilyIndices,
                     },
                     allocationCreateInfo} {
    std::ranges::copy(std::forward<R>(r),
                      static_cast<std::ranges::range_value_t<R>*>(data));
  }

  MappedBuffer() = default;

  MappedBuffer(const MappedBuffer&) = delete;
  MappedBuffer& operator=(const MappedBuffer&) = delete;

  MappedBuffer(MappedBuffer&& src) noexcept = default;

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
  [[nodiscard]] auto as_range(std::size_t byteOffset = 0) const
      -> std::span<const T> {
    assert(byteOffset <= size && "Out of bound: byteOffset > size");
    return {
        reinterpret_cast<const T*>(static_cast<const char*>(data) + byteOffset),
        (size - byteOffset) / sizeof(T)};
  }

  template <typename T>
  [[nodiscard]] auto as_range(std::size_t byteOffset = 0) -> std::span<T> {
    assert(byteOffset <= size && "Out of bound: byteOffset > size");
    return {reinterpret_cast<T*>(static_cast<char*>(data) + byteOffset),
            (size - byteOffset) / sizeof(T)};
  }

  template <typename T>
  [[nodiscard]] auto as_value(std::size_t byteOffset = 0) const -> const T& {
    assert(byteOffset + sizeof(T) <= size &&
           "Out of bound: byteOffset + sizeof(T) > size");
    return *reinterpret_cast<const T*>(static_cast<const char*>(data) +
                                       byteOffset);
  }

  template <typename T>
  [[nodiscard]] auto as_value(std::size_t byteOffset = 0) -> T& {
    assert(byteOffset + sizeof(T) <= size &&
           "Out of bound: byteOffset + sizeof(T) > size");
    return *reinterpret_cast<T*>(static_cast<char*>(data) + byteOffset);
  }

  [[nodiscard]] auto unmap() && noexcept {
    allocator.unmapMemory(allocation);
    return static_cast<AllocatedBuffer>(std::move(*this));
  }

 private:
  explicit MappedBuffer(AllocatedBuffer&& allocatedBuffer)
      : AllocatedBuffer{std::move(allocatedBuffer)},
        data{allocator.mapMemory(allocation)} {}
};

};  // namespace vkit::graphics
