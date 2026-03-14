#pragma once

#include "vulkan/descriptor_set_layout/dsl.hpp"

namespace vkit::vulkan::dsl {
struct MaterialLayout final : DescriptorSetLayout {
  static constexpr auto kMaterialBindingIdx = std::uint32_t{0};

  static constexpr auto kBindings =
      std::array<vk::DescriptorSetLayoutBinding, 1>{
          layoutBinding(kMaterialBindingIdx, vk::DescriptorType::eStorageBuffer,
                        1, vk::ShaderStageFlagBits::eFragment),
      };

  explicit MaterialLayout(vk::Device device)
      : DescriptorSetLayout{
            device,
            vk::StructureChain{
                vk::DescriptorSetLayoutCreateInfo{
                    vk::DescriptorSetLayoutCreateFlagBits::eUpdateAfterBindPool,
                    kBindings,
                },
            }
                .get()} {}
};
};  // namespace vkit::vulkan::dsl
