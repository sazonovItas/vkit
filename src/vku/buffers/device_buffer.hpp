#pragma once

#include <ranges>

#include "vku/buffers/allocated_buffer.hpp"
#include "vku/buffers/mapped_buffer.hpp"
#include "vku/commands.hpp"
#include "vku/constants.hpp"

namespace vku {
class DeviceBuffer : public AllocatedBuffer {
 public:
  DeviceBuffer(vma::Allocator allocator,
               const vk::BufferCreateInfo& create_info,
               const vma::AllocationCreateInfo& allocation_create_info =
                   allocation::kDeviceLocal)
      : AllocatedBuffer(allocator, create_info, allocation_create_info) {}

  DeviceBuffer() = default;

  DeviceBuffer(const DeviceBuffer&) = delete;
  DeviceBuffer& operator=(const DeviceBuffer&) = delete;

  DeviceBuffer(DeviceBuffer&& src) noexcept = default;

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
  void update(vk::Device device, vk::CommandPool command_pool, vk::Queue queue,
              std::from_range_t, R&& r) {
    auto total_size = r.size() * sizeof(std::ranges::range_value_t<R>);
    assert(total_size <= this->size &&
           "Buffer size exceeded device buffer size.");

    auto stage_buffer = MappedBuffer{
        allocator,
        std::from_range_t{},
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

    execute_command_and_wait(device, command_pool, queue, k_copy_buffer);
  }
};
};  // namespace vku
