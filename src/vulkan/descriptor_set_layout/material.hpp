#pragma once

#include "vulkan/descriptor_set_layout/dsl.hpp"

namespace vkit::vulkan::dsl {
struct Material final : DescriptorSetLayout {
  static constexpr auto kMaxMaterials = std::uint32_t{256};

  static constexpr auto kMaterialBindingIdx = std::uint32_t{0};

  static constexpr auto kBindings =
      std::array<vk::DescriptorSetLayoutBinding, 1>{
          layoutBinding(0, vk::DescriptorType::eUniformBuffer, kMaxMaterials,
                        vk::ShaderStageFlagBits::eFragment),
      };

  static constexpr auto kBindingFlags =
      std::array<vk::DescriptorBindingFlags, 1>{
          vk::DescriptorBindingFlagBits::ePartiallyBound |
              vk::DescriptorBindingFlagBits::eUpdateAfterBind |
              vk::DescriptorBindingFlagBits::eVariableDescriptorCount,
      };

  explicit Material(vk::Device device)
      : DescriptorSetLayout{
            device,
            vk::StructureChain{
                vk::DescriptorSetLayoutCreateInfo{
                    vk::DescriptorSetLayoutCreateFlagBits::eUpdateAfterBindPool,
                    kBindings,
                },
                vk::DescriptorSetLayoutBindingFlagsCreateInfo{
                    kBindingFlags,
                },
            }
                .get(),
        } {}
};
};  // namespace vkit::vulkan::dsl
