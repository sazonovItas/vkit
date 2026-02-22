#pragma once

namespace vkit::vulkan::dsl {
constexpr auto layout_binding(std::uint32_t binding,
                              vk::DescriptorType const type,
                              std::uint32_t count,
                              vk::ShaderStageFlagBits stage) {
  return vk::DescriptorSetLayoutBinding{binding, type, count, stage};
}
};  // namespace vkit::vulkan::dsl
