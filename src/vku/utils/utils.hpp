#pragma once

#include "vku/images/image.hpp"

namespace vku {
struct Bitmap {
  vk::Extent2D extent;
  std::span<const std::byte> bytes;
};

struct ImageBarrierInfo {
  vk::ImageLayout srcLayout;
  vk::AccessFlags2 srcAccess;
  vk::PipelineStageFlagBits2 srcStage;
  vk::ImageLayout dstLayout;
  vk::AccessFlags2 dstAccess;
  vk::PipelineStageFlagBits2 dstStage;
};

struct DeviceCopyInfo {
  vk::Device device;
  vk::CommandPool commandPool;
  vk::Queue queue;
};

template <typename T>
[[nodiscard]] constexpr auto contains(vk::Flags<T> flags, T flag) noexcept
    -> bool {
  if constexpr (vk::FlagTraits<T>::isBitmask) {
    return static_cast<bool>(flags & flag);
  } else {
    return (flags & flag) == flag;
  }
}

template <typename T>
[[nodiscard]] constexpr auto contains(vk::Flags<T> super,
                                      vk::Flags<T> sub) noexcept -> bool {
  return (super & sub) == sub;
}

[[nodiscard]] constexpr auto toExtent2D(const vk::Extent3D& extent) noexcept
    -> vk::Extent2D {
  return {extent.width, extent.height};
}

[[nodiscard]] constexpr auto toExtent3D(const vk::Extent2D& extent) noexcept
    -> vk::Extent3D {
  return {extent.width, extent.height, 1};
}

template <std::floating_point T = float>
[[nodiscard]] constexpr auto aspect(const vk::Extent2D& extent) noexcept -> T {
  assert(extent.height != 0 && "Height must be nonzero.");
  return static_cast<T>(extent.width) / static_cast<T>(extent.height);
}

void generateMipmaps(vk::CommandBuffer cb, const Image& image,
                     const ImageBarrierInfo& barrierInfo,
                     std::uint32_t baseArrayLayer = 0);
}  // namespace vku
