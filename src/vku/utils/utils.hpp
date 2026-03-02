#pragma once

namespace vku {
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

[[nodiscard]] constexpr auto to_extent_2d(const vk::Extent3D& extent) noexcept
    -> vk::Extent2D {
  return {extent.width, extent.height};
}

[[nodiscard]] constexpr auto to_extent_3d(const vk::Extent2D& extent) noexcept
    -> vk::Extent3D {
  return {extent.width, extent.height, 1};
}

template <std::floating_point T = float>
[[nodiscard]] constexpr auto aspect(const vk::Extent2D& extent) noexcept -> T {
  assert(extent.height != 0 && "Height must be nonzero.");
  return static_cast<T>(extent.width) / static_cast<T>(extent.height);
}
}  // namespace vku
