#pragma once

namespace vkit::vulkan::dsl {
constexpr auto layoutBinding(std::uint32_t binding, vk::DescriptorType type,
                             std::uint32_t count,
                             vk::Flags<vk::ShaderStageFlagBits> stage) {
  return vk::DescriptorSetLayoutBinding{binding, type, count, stage};
}

struct DescriptorSetLayout : public vk::UniqueDescriptorSetLayout {
  DescriptorSetLayout(vk::Device device,
                      const vk::DescriptorSetLayoutCreateInfo& createInfo)
      : vk::UniqueDescriptorSetLayout{
            device.createDescriptorSetLayoutUnique(createInfo),
        } {}
};
};  // namespace vkit::vulkan::dsl
