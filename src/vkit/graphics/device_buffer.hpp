#pragma once

#include <vk_mem_alloc.hpp>

#include "vkit/graphics/buffer.hpp"
#include "vkit/graphics/device.hpp"
#include "vkit/graphics/mapped_buffer.hpp"
#include "vkit/graphics/util.hpp"

namespace vkit::graphics {

class DeviceBuffer : public AllocatedBuffer {
 public:
  DeviceBuffer(vma::Allocator allocator, const vk::BufferCreateInfo& createInfo,
               const vma::AllocationCreateInfo& allocationCreateInfo =
                   allocation::kDeviceLocal)
      : AllocatedBuffer(allocator, createInfo, allocationCreateInfo) {}

  DeviceBuffer(const GFXDevice& gfxDevice, vk::Buffer srcBuffer,
               std::size_t offset, vk::DeviceSize size,
               vk::Flags<vk::BufferUsageFlagBits> usage,
               const vma::AllocationCreateInfo& allocationCreateInfo =
                   allocation::kDeviceLocal)
      : AllocatedBuffer(gfxDevice.getAllocator(),
                        vk::BufferCreateInfo{
                            {},
                            size,
                            usage | vk::BufferUsageFlagBits::eTransferDst,
                        },
                        allocationCreateInfo) {
    copy(gfxDevice, srcBuffer, offset, {}, size);
  }

  DeviceBuffer(const GFXDevice& gfxDevice, vk::Buffer srcBuffer,
               std::size_t offset, vk::DeviceSize size,
               vk::Flags<vk::BufferUsageFlagBits> usage,
               vk::ArrayProxy<const std::uint32_t> queueFamilyIndices,
               const vma::AllocationCreateInfo& allocationCreateInfo =
                   allocation::kDeviceLocal)
      : AllocatedBuffer(gfxDevice.getAllocator(),
                        vk::BufferCreateInfo{
                            {},
                            size,
                            usage | vk::BufferUsageFlagBits::eTransferDst,
                            util::getSharingMode(queueFamilyIndices),
                            queueFamilyIndices,
                        },
                        allocationCreateInfo) {
    copy(gfxDevice, srcBuffer, offset, {}, size);
  }

  template <std::ranges::input_range R>
    requires(std::ranges::sized_range<R> &&
             std::is_trivially_copyable_v<std::ranges::range_value_t<R>>)
  DeviceBuffer(const GFXDevice& gfxDevice, std::from_range_t fromRange, R&& r,
               vk::Flags<vk::BufferUsageFlagBits> usage,
               const vma::AllocationCreateInfo& allocationCreateInfo =
                   allocation::kDeviceLocal)
      : AllocatedBuffer(gfxDevice.getAllocator(),
                        vk::BufferCreateInfo{
                            {},
                            r.size() * sizeof(std::ranges::range_value_t<R>),
                            usage | vk::BufferUsageFlagBits::eTransferDst,
                        },
                        allocationCreateInfo) {
    update(gfxDevice, fromRange, r);
  }

  template <std::ranges::input_range R>
    requires(std::ranges::sized_range<R> &&
             std::is_trivially_copyable_v<std::ranges::range_value_t<R>>)
  DeviceBuffer(const GFXDevice& gfxDevice, std::from_range_t fromRange, R&& r,
               vk::Flags<vk::BufferUsageFlagBits> usage,
               vk::ArrayProxy<const std::uint32_t> queueFamilyIndices,
               const vma::AllocationCreateInfo& allocationCreateInfo =
                   allocation::kDeviceLocal)
      : AllocatedBuffer(gfxDevice.getAllocator(),
                        vk::BufferCreateInfo{
                            {},
                            r.size() * sizeof(std::ranges::range_value_t<R>),
                            usage | vk::BufferUsageFlagBits::eTransferDst,
                            util::getSharingMode(queueFamilyIndices),
                            queueFamilyIndices,
                        },
                        allocationCreateInfo) {
    update(gfxDevice, fromRange, r);
  }

  DeviceBuffer() = default;

  DeviceBuffer(const DeviceBuffer&) = delete;
  DeviceBuffer& operator=(const DeviceBuffer&) = delete;

  DeviceBuffer(DeviceBuffer&& src) noexcept = default;

  DeviceBuffer& operator=(DeviceBuffer&& src) noexcept {
    static_cast<AllocatedBuffer&>(*this) =
        std::move(static_cast<AllocatedBuffer&>(src));
    return *this;
  };

  [[nodiscard]] auto getAddress(vk::Device device) const noexcept
      -> vk::DeviceAddress {
    return device.getBufferAddress(vk::BufferDeviceAddressInfo{buffer});
  }

  template <std::ranges::input_range R>
    requires(std::ranges::sized_range<R> &&
             std::is_trivially_copyable_v<std::ranges::range_value_t<R>>)
  void update(const GFXDevice& gfxDevice, std::from_range_t fromRange, R&& r,
              std::size_t deviceBufferOffset = {}) {
    auto total_size = r.size() * sizeof(std::ranges::range_value_t<R>);
    assert(deviceBufferOffset + total_size <= this->size &&
           "Total size exceedes device buffer size.");

    auto stage_buffer = MappedBuffer{
        allocator,
        fromRange,
        std::forward<R>(r),
        vk::BufferUsageFlagBits::eTransferSrc,
    };

    copy(gfxDevice, stage_buffer.buffer, {}, deviceBufferOffset, total_size);
  }

  template <std::ranges::input_range R>
    requires(std::ranges::sized_range<R> &&
             std::is_trivially_copyable_v<std::ranges::range_value_t<R>>)
  void update(const GFXDevice& gfxDevice, std::from_range_t fromRange, R&& r,
              vk::ArrayProxy<const std::uint32_t> queueFamilyIndices,
              std::size_t deviceBufferOffset = {}) {
    auto total_size = r.size() * sizeof(std::ranges::range_value_t<R>);
    assert(deviceBufferOffset + total_size <= this->size &&
           "Total size exceedes device buffer size.");

    auto stage_buffer = MappedBuffer{
        allocator,          fromRange,
        std::forward<R>(r), vk::BufferUsageFlagBits::eTransferSrc,
        queueFamilyIndices,
    };

    copy(gfxDevice, stage_buffer.buffer, {}, deviceBufferOffset, total_size);
  }

 private:
  explicit DeviceBuffer(AllocatedBuffer&& allocatedBuffer)
      : AllocatedBuffer{std::move(allocatedBuffer)} {}

  void copy(const GFXDevice& gfxDevice, vk::Buffer srcBuffer,
            std::size_t srcOffset, std::size_t dstOffset, std::size_t size);
};

};  // namespace vkit::graphics
