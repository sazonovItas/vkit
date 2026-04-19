#pragma once

#include <chrono>

namespace vkit::graphics::util {

inline void requireSuccess(const vk::Result result, const char* msg) {
  if (result != vk::Result::eSuccess) {
    throw std::runtime_error{msg};
  }
}

template <typename T, typename E>
[[nodiscard]] inline auto contains(T flags, E bit) -> bool {
  return (flags & bit) == bit;
}

[[nodiscard]] inline auto getSharingMode(
    vk::ArrayProxy<const std::uint32_t> queueFamilyIndices) -> vk::SharingMode {
  return queueFamilyIndices.size() <= 1 ? vk::SharingMode::eExclusive
                                        : vk::SharingMode::eConcurrent;
}

void recordAndSubmit(vk::Device device, vk::Queue queue, vk::CommandPool cp,
                     std::function<void(vk::CommandBuffer)>&& record,
                     std::chrono::nanoseconds waitTime = std::chrono::seconds{
                         30});

};  // namespace vkit::graphics::util
