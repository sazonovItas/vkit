#pragma once

namespace vku {
struct Buffer {
  vk::Buffer buffer;
  vk::DeviceSize size;

  [[nodiscard]] explicit operator vk::Buffer() const noexcept { return buffer; }

  [[nodiscard]] auto get_create_info(
      vk::Format format, vk::DeviceSize offset = 0,
      vk::DeviceSize range = vk::WholeSize) const noexcept
      -> vk::BufferViewCreateInfo {
    return {{}, buffer, format, offset, range};
  }
};
}  // namespace vku
