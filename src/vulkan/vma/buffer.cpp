#include "buffer.hpp"

#include <numeric>
#include <print>

namespace vkit::vulkan::vma {
void BufferDeleter::operator()(RawBuffer const& raw_buffer) const noexcept {
  vmaDestroyBuffer(raw_buffer.allocator, raw_buffer.buffer,
                   raw_buffer.allocation);
}

auto create_buffer(const BufferCreateInfo& create_info,
                   BufferMemoryType memory_type, vk::DeviceSize size)
    -> Buffer {
  if (0 == size) {
    // FIX: migrate to exceptions instead of just logging
    std::println(stderr, "Buffer cannot be 0-sized");
    return {};
  }

  auto allocation_ci = VmaAllocationCreateInfo{};
  allocation_ci.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
  auto usage = create_info.usage;
  if (memory_type == BufferMemoryType::kDevice) {
    allocation_ci.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
    usage |= vk::BufferUsageFlagBits::eTransferDst;
  } else {
    allocation_ci.usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
    allocation_ci.flags |= VMA_ALLOCATION_CREATE_MAPPED_BIT;
  }

  auto buffer_ci = vk::BufferCreateInfo{};
  buffer_ci.setQueueFamilyIndices(create_info.queue_family)
      .setSize(size)
      .setUsage(usage);

  auto vma_buffer_ci = static_cast<VkBufferCreateInfo>(buffer_ci);

  VmaAllocation allocation{};
  VkBuffer buffer{};
  auto allocation_info = VmaAllocationInfo{};
  auto const result =
      vmaCreateBuffer(create_info.allocator, &vma_buffer_ci, &allocation_ci,
                      &buffer, &allocation, &allocation_info);
  if (result != VK_SUCCESS) {
    // FIX: migrate to exceptions instead of just logging
    std::println(stderr, "failed to create VMA Buffer");
    return {};
  }

  auto address = vk::DeviceAddress{0};
  if (memory_type == BufferMemoryType::kDevice &&
      (usage & vk::BufferUsageFlagBits::eShaderDeviceAddress) ==
          vk::BufferUsageFlagBits::eShaderDeviceAddress) {
    auto device_address_info = vk::BufferDeviceAddressInfo{buffer};
    address = create_info.device.getBufferAddress(device_address_info);
  }

  return RawBuffer{
      .allocator = create_info.allocator,
      .allocation = allocation,
      .buffer = buffer,
      .size = size,
      .address = address,
      .mapped = allocation_info.pMappedData,
  };
}

auto create_device_buffer(BufferCreateInfo const& create_info,
                          CommandBlock command_block,
                          ByteSpans const& byte_spans) -> Buffer {
  auto const total_size = std::accumulate(
      byte_spans.begin(), byte_spans.end(), 0UZ,
      [](std::size_t const n, std::span<std::byte const> bytes) {
        return n + bytes.size();
      });

  auto staging_ci = create_info;
  staging_ci.usage = vk::BufferUsageFlagBits::eTransferSrc;

  auto staging_buffer =
      create_buffer(staging_ci, BufferMemoryType::kHost, total_size);
  auto ret = create_buffer(create_info, BufferMemoryType::kDevice, total_size);
  if (!staging_buffer.get().buffer || !ret.get().buffer) {
    return {};
  }

  auto dst = staging_buffer.get().mapped_span();
  for (auto const bytes : byte_spans) {
    std::memcpy(dst.data(), bytes.data(), bytes.size());
    dst = dst.subspan(bytes.size());
  }

  auto buffer_copy = vk::BufferCopy2{};
  buffer_copy.setSize(total_size);
  auto copy_buffer_info = vk::CopyBufferInfo2{};
  copy_buffer_info.setSrcBuffer(staging_buffer.get().buffer)
      .setDstBuffer(ret.get().buffer)
      .setRegions(buffer_copy);

  command_block.cb().copyBuffer2(copy_buffer_info);

  command_block.submit_and_wait();

  return ret;
}
};  // namespace vkit::vulkan::vma
