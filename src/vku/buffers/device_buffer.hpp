#pragma once

#include <ranges>

#include "vku/buffers/allocated_buffer.hpp"
#include "vku/buffers/mapped_buffer.hpp"
#include "vku/commands.hpp"
#include "vku/constants.hpp"

namespace vku {
class DeviceBuffer : AllocatedBuffer {
 public:
  DeviceBuffer(vma::Allocator allocator,
               const vk::BufferCreateInfo& create_info,
               const vma::AllocationCreateInfo& allocation_create_info =
                   allocation::kDeviceLocal)
      : AllocatedBuffer(allocator, create_info, allocation_create_info) {}

  DeviceBuffer(const DeviceBuffer&) = delete;

  DeviceBuffer(DeviceBuffer&& src) noexcept = default;

  DeviceBuffer& operator=(const DeviceBuffer&) = delete;

  DeviceBuffer& operator=(DeviceBuffer&& src) noexcept {
    static_cast<AllocatedBuffer&>(*this) =
        std::move(static_cast<AllocatedBuffer&>(src));
    return *this;
  };

  [[nodiscard]] auto get_address(vk::Device device) const noexcept
      -> vk::DeviceAddress {
    return device.getBufferAddress(vk::BufferDeviceAddressInfo{buffer});
  }

  template <std::ranges::input_range R>
    requires(std::ranges::sized_range<R> &&
             std::is_trivially_copyable_v<std::ranges::range_value_t<R>>)
  void copy_from(vk::Device device, vk::CommandPool command_pool,
                 vk::Queue queue, R&& r) {
    auto total_size = r.size() * sizeof(std::ranges::range_value_t<R>);
    assert(total_size > this->size &&
           "Bytes size exceeded device buffer size.");

    auto stage_buffer = MappedBuffer{
        allocator,
        std::from_range_t{},
        std::forward<R>(r),
        vk::BufferUsageFlagBits::eTransferSrc,
    };

    vku::execute_command(
        device, command_pool, queue, [&](vk::CommandBuffer cb) {
          auto buffer_copy = vk::BufferCopy2{}.setSize(total_size);
          auto copy_buffer_info = vk::CopyBufferInfo2{}
                                      .setSrcBuffer(stage_buffer.buffer)
                                      .setDstBuffer(buffer)
                                      .setRegions(buffer_copy);

          cb.copyBuffer2(copy_buffer_info);
        });
  }
};
};  // namespace vku
