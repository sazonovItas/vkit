#pragma once

namespace vkit::vulkan::dsl {
struct DescriptorSetLayout : public vk::UniqueDescriptorSetLayout {
  explicit DescriptorSetLayout(vk::Device device,
                               vk::DescriptorSetLayoutCreateInfo createInfo)
      : vk::UniqueDescriptorSetLayout{
            device.createDescriptorSetLayoutUnique(createInfo),
        } {}
};

constexpr auto layoutBinding(std::uint32_t binding, vk::DescriptorType type,
                             std::uint32_t cnt, vk::ShaderStageFlagBits stage) {
  return vk::DescriptorSetLayoutBinding{binding, type, cnt, stage};
}
};  // namespace vkit::vulkan::dsl
