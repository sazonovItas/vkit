#pragma once

namespace vkit::graphics {

struct DescriptorSetLayout : public vk::UniqueDescriptorSetLayout {
  DescriptorSetLayout(vk::Device device,
                      const vk::DescriptorSetLayoutCreateInfo& createInfo)
      : vk::UniqueDescriptorSetLayout{
            device.createDescriptorSetLayoutUnique(createInfo),
        } {}
};

};  // namespace vkit::graphics
