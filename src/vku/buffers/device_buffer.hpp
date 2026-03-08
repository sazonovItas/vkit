#pragma once

#include <ranges>

#include "vku/buffers/allocated_buffer.hpp"
#include "vku/buffers/mapped_buffer.hpp"
#include "vku/commands.hpp"
#include "vku/constants.hpp"
#include "vku/queue.hpp"
#include "vku/utils/utils.hpp"

namespace vku {
class DeviceBuffer : public AllocatedBuffer {
 public:
  DeviceBuffer(vma::Allocator allocator, const vk::BufferCreateInfo& createInfo,
               const vma::AllocationCreateInfo& allocationCreateInfo =
                   allocation::kDeviceLocal)
      : AllocatedBuffer(allocator, createInfo, allocationCreateInfo) {}

  DeviceBuffer(vma::Allocator allocator, const DeviceCopyInfo& cmdCopyInfo,
               vk::Buffer srcBuffer, vk::DeviceSize offset, vk::DeviceSize size,
               vk::Flags<vk::BufferUsageFlagBits> usage,
               const vma::AllocationCreateInfo& allocationCreateInfo =
                   allocation::kDeviceLocal)
      : AllocatedBuffer(allocator,
                        vk::BufferCreateInfo{
                            {},
                            size,
                            usage | vk::BufferUsageFlagBits::eTransferDst,
                        },
                        allocationCreateInfo) {
    copy(cmdCopyInfo, srcBuffer, offset, {}, size);
  }

  DeviceBuffer(vma::Allocator allocator, const DeviceCopyInfo& cmdCopyInfo,
               vk::Buffer srcBuffer, vk::DeviceSize offset, vk::DeviceSize size,
               vk::Flags<vk::BufferUsageFlagBits> usage,
               vk::ArrayProxy<const std::uint32_t> queueFamilyIndices,
               const vma::AllocationCreateInfo& allocationCreateInfo =
                   allocation::kDeviceLocal)
      : AllocatedBuffer(allocator,
                        vk::BufferCreateInfo{
                            {},
                            size,
                            usage | vk::BufferUsageFlagBits::eTransferDst,
                            getSharingMode(queueFamilyIndices),
                            queueFamilyIndices,
                        },
                        allocationCreateInfo) {
    copy(cmdCopyInfo, srcBuffer, offset, {}, size);
  }

  template <std::ranges::input_range R>
    requires(std::ranges::sized_range<R> &&
             std::is_trivially_copyable_v<std::ranges::range_value_t<R>>)
  DeviceBuffer(vma::Allocator allocator, const DeviceCopyInfo& cmdCopyInfo,
               std::from_range_t fromRange, R&& r,
               vk::Flags<vk::BufferUsageFlagBits> usage,
               const vma::AllocationCreateInfo& allocationCreateInfo =
                   allocation::kDeviceLocal)
      : AllocatedBuffer(allocator,
                        vk::BufferCreateInfo{
                            {},
                            r.size() * sizeof(std::ranges::range_value_t<R>),
                            usage | vk::BufferUsageFlagBits::eTransferDst,
                        },
                        allocationCreateInfo) {
    update(cmdCopyInfo, fromRange, r);
  }

  template <std::ranges::input_range R>
    requires(std::ranges::sized_range<R> &&
             std::is_trivially_copyable_v<std::ranges::range_value_t<R>>)
  DeviceBuffer(vma::Allocator allocator, const DeviceCopyInfo& cmdCopyInfo,
               std::from_range_t fromRange, R&& r,
               vk::Flags<vk::BufferUsageFlagBits> usage,
               vk::ArrayProxy<const std::uint32_t> queueFamilyIndices,
               const vma::AllocationCreateInfo& allocationCreateInfo =
                   allocation::kDeviceLocal)
      : AllocatedBuffer(allocator,
                        vk::BufferCreateInfo{
                            {},
                            r.size() * sizeof(std::ranges::range_value_t<R>),
                            usage | vk::BufferUsageFlagBits::eTransferDst,
                            getSharingMode(queueFamilyIndices),
                            queueFamilyIndices,
                        },
                        allocationCreateInfo) {
    update(cmdCopyInfo, fromRange, r);
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
  void update(const DeviceCopyInfo& cmdCopyInfo, std::from_range_t fromRange,
              R&& r, vk::DeviceSize deviceBufferOffset = {}) {
    auto total_size = r.size() * sizeof(std::ranges::range_value_t<R>);
    assert(deviceBufferOffset + total_size <= this->size &&
           "Total size exceedes device buffer size.");

    auto stage_buffer = MappedBuffer{
        allocator,
        fromRange,
        std::forward<R>(r),
        vk::BufferUsageFlagBits::eTransferSrc,
    };

    copy(cmdCopyInfo, stage_buffer.buffer, {}, deviceBufferOffset, total_size);
  }

  template <std::ranges::input_range R>
    requires(std::ranges::sized_range<R> &&
             std::is_trivially_copyable_v<std::ranges::range_value_t<R>>)
  void update(const DeviceCopyInfo& cmdCopyInfo, std::from_range_t fromRange,
              R&& r, vk::ArrayProxy<const std::uint32_t> queueFamilyIndices,
              vk::DeviceSize deviceBufferOffset = {}) {
    auto total_size = r.size() * sizeof(std::ranges::range_value_t<R>);
    assert(deviceBufferOffset + total_size <= this->size &&
           "Total size exceedes device buffer size.");

    auto stage_buffer = MappedBuffer{
        allocator,          fromRange,
        std::forward<R>(r), vk::BufferUsageFlagBits::eTransferSrc,
        queueFamilyIndices,
    };

    copy(cmdCopyInfo, stage_buffer.buffer, {}, deviceBufferOffset, total_size);
  }

 private:
  explicit DeviceBuffer(AllocatedBuffer&& allocatedBuffer)
      : AllocatedBuffer{std::move(allocatedBuffer)} {}

  void copy(const DeviceCopyInfo& cmdCopyInfo, vk::Buffer srcBuffer,
            vk::DeviceSize srcOffset, vk::DeviceSize dstOffset,
            vk::DeviceSize size) {
    auto copy_buffer_fn = [&](vk::CommandBuffer cb) {
      auto buffer_copy = vk::BufferCopy2{};
      buffer_copy.setSize(size).setSrcOffset(srcOffset).setDstOffset(dstOffset);
      auto copy_buffer_info = vk::CopyBufferInfo2{}
                                  .setSrcBuffer(srcBuffer)
                                  .setDstBuffer(buffer)
                                  .setRegions(buffer_copy);

      cb.copyBuffer2(copy_buffer_info);
    };

    executeCommandAndWait(cmdCopyInfo.device, cmdCopyInfo.commandPool,
                          cmdCopyInfo.queue, copy_buffer_fn);
  }
};
};  // namespace vku
