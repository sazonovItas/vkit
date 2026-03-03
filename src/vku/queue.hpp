#pragma once

namespace vku {
[[nodiscard]] inline auto getSharingMode(
    vk::ArrayProxy<const std::uint32_t> queueFamilyIndices) -> vk::SharingMode {
  return queueFamilyIndices.size() == 1 ? vk::SharingMode::eExclusive
                                        : vk::SharingMode::eConcurrent;
}
};  // namespace vku
