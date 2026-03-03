#pragma once

#include <ranges>

#include "vku/buffers/allocated_buffer.hpp"
#include "vku/buffers/mapped_buffer.hpp"
#include "vku/commands.hpp"
#include "vku/constants.hpp"
#include "vku/queue.hpp"
#include "vku/utils/utils.hpp"
#include "vulkan/vulkan.hpp"

namespace vku {
class DeviceBuffer : public AllocatedBuffer {
 public:
  DeviceBuffer(vma::Allocator allocator, const vk::BufferCreateInfo& createInfo,
               const vma::AllocationCreateInfo& allocationCreateInfo =
                   allocation::kDeviceLocal)
      : AllocatedBuffer(allocator, createInfo, allocationCreateInfo) {}

  template <std::ranges::input_range R>
    requires(std::ranges::sized_range<R> &&
             std::is_trivially_copyable_v<std::ranges::range_value_t<R>>)
  DeviceBuffer(vma::Allocator allocator, const DeviceCopyInfo& cmdCopyInfo,
               std::from_range_t fromRange, R&& r,
               vk::BufferUsageFlagBits usage,
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
               vk::BufferUsageFlagBits usage,
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
              R&& r) {
    auto total_size = r.size() * sizeof(std::ranges::range_value_t<R>);
    assert(total_size <= this->size &&
           "Total size exceedes device buffer size.");

    auto stage_buffer = MappedBuffer{
        allocator,
        fromRange,
        std::forward<R>(r),
        vk::BufferUsageFlagBits::eTransferSrc,
    };

    auto k_copy_buffer = [&](vk::CommandBuffer cb) {
      auto buffer_copy = vk::BufferCopy2{}.setSize(total_size);
      auto copy_buffer_info = vk::CopyBufferInfo2{}
                                  .setSrcBuffer(stage_buffer.buffer)
                                  .setDstBuffer(buffer)
                                  .setRegions(buffer_copy);

      cb.copyBuffer2(copy_buffer_info);
    };

    executeCommandAndWait(cmdCopyInfo.device, cmdCopyInfo.commandPool,
                          cmdCopyInfo.queue, k_copy_buffer);
  }

  template <std::ranges::input_range R>
    requires(std::ranges::sized_range<R> &&
             std::is_trivially_copyable_v<std::ranges::range_value_t<R>>)
  void update(const DeviceCopyInfo& cmdCopyInfo, std::from_range_t fromRange,
              R&& r, vk::ArrayProxy<const std::uint32_t> queueFamilyIndices) {
    auto total_size = r.size() * sizeof(std::ranges::range_value_t<R>);
    assert(total_size <= this->size &&
           "Total size exceedes device buffer size.");

    auto stage_buffer = MappedBuffer{
        allocator,          fromRange,
        std::forward<R>(r), vk::BufferUsageFlagBits::eTransferSrc,
        queueFamilyIndices,
    };

    auto k_copy_buffer = [&](vk::CommandBuffer cb) {
      auto buffer_copy = vk::BufferCopy2{}.setSize(total_size);
      auto copy_buffer_info = vk::CopyBufferInfo2{}
                                  .setSrcBuffer(stage_buffer.buffer)
                                  .setDstBuffer(buffer)
                                  .setRegions(buffer_copy);

      cb.copyBuffer2(copy_buffer_info);
    };

    executeCommandAndWait(cmdCopyInfo.device, cmdCopyInfo.commandPool,
                          cmdCopyInfo.queue, k_copy_buffer);
  }
};
};  // namespace vku
