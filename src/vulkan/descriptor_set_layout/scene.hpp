#pragma once

#include "vulkan/descriptor_set_layout/dsl.hpp"

namespace vkit::vulkan::dsl {
struct SceneLayout final : DescriptorSetLayout {
  static constexpr auto kUBOBindingIdx = std::uint32_t{0};
  static constexpr auto kUBOParamsBindingIdx = std::uint32_t{1};
  static constexpr auto kSSBOLightsBindingIdx = std::uint32_t{2};

  static constexpr auto kBindings =
      std::array<vk::DescriptorSetLayoutBinding, 3>{
          layoutBinding(kUBOBindingIdx, vk::DescriptorType::eUniformBuffer, 1,
                        vk::ShaderStageFlagBits::eVertex |
                            vk::ShaderStageFlagBits::eFragment),
          layoutBinding(kUBOParamsBindingIdx,
                        vk::DescriptorType::eUniformBuffer, 1,
                        vk::ShaderStageFlagBits::eVertex |
                            vk::ShaderStageFlagBits::eFragment),
          layoutBinding(kSSBOLightsBindingIdx,
                        vk::DescriptorType::eStorageBuffer, 1,
                        vk::ShaderStageFlagBits::eFragment),
      };

  explicit SceneLayout(vk::Device device)
      : DescriptorSetLayout{
            device,
            vk::StructureChain{
                vk::DescriptorSetLayoutCreateInfo{
                    {},
                    kBindings,
                },
            }
                .get(),
        } {}
};
};  // namespace vkit::vulkan::dsl
