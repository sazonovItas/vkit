#pragma once

#include "vulkan/descriptor_set_layout/dsl.hpp"

namespace vkit::vulkan::dsl {
struct MaterialLayout final : DescriptorSetLayout {
  static constexpr auto kMaterialBindingIdx = std::uint32_t{0};

  static constexpr auto kBindings =
      std::array<vk::DescriptorSetLayoutBinding, 1>{
          layoutBinding(0, vk::DescriptorType::eStorageBuffer, 1,
                        vk::ShaderStageFlagBits::eAllGraphics),
      };

  static constexpr auto kBindingFlags =
      std::array<vk::DescriptorBindingFlags, 1>{
          vk::DescriptorBindingFlagBits::eUpdateAfterBind,
      };

  explicit MaterialLayout(vk::Device device)
      : DescriptorSetLayout{
            device,
            vk::StructureChain{
                vk::DescriptorSetLayoutCreateInfo{
                    vk::DescriptorSetLayoutCreateFlagBits::eUpdateAfterBindPool,
                    kBindings,
                },
                vk::DescriptorSetLayoutBindingFlagsCreateInfo{kBindingFlags},
            }
                .get()} {}
};
};  // namespace vkit::vulkan::dsl
