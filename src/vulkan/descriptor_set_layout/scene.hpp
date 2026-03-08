#pragma once

#include "vulkan/descriptor_set_layout/dsl.hpp"

namespace vkit::vulkan::dsl {
struct Scene final : DescriptorSetLayout {
  static constexpr auto kUBOBindingIdx = std::uint32_t{0};
  static constexpr auto kUBOParamsBindingIdx = std::uint32_t{1};

  static constexpr auto kBindings =
      std::array<vk::DescriptorSetLayoutBinding, 2>{
          layoutBinding(0, vk::DescriptorType::eUniformBuffer, 1,
                        vk::ShaderStageFlagBits::eVertex |
                            vk::ShaderStageFlagBits::eFragment),
          layoutBinding(1, vk::DescriptorType::eUniformBuffer, 1,
                        vk::ShaderStageFlagBits::eVertex |
                            vk::ShaderStageFlagBits::eFragment),
      };

  explicit Scene(vk::Device device)
      : DescriptorSetLayout{
            device,
            vk::DescriptorSetLayoutCreateInfo{
                {},
                kBindings,
            },
        } {}
};
};  // namespace vkit::vulkan::dsl
