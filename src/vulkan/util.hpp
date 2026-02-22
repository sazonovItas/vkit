#pragma once

namespace vkit::vulkan::util {
template <typename T, typename E>
auto contains(T flags, E bit) -> bool {
  return (flags & bit) == bit;
}
};  // namespace vkit::vulkan::util
