#pragma once

namespace vku {
[[nodiscard]] inline auto get_sharing_mode(
    vk::ArrayProxy<const std::uint32_t> queue_family_indices)
    -> vk::SharingMode {
  return queue_family_indices.size() == 1 ? vk::SharingMode::eExclusive
                                          : vk::SharingMode::eConcurrent;
}
};  // namespace vku
