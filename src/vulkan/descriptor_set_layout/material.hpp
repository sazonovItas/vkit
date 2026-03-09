#pragma once

#include "vulkan/descriptor_set_layout/dsl.hpp"

namespace vkit::vulkan::dsl {
struct MaterialLayout final : DescriptorSetLayout {
  struct alignas(16) ShaderMaterial {};

  static constexpr auto kMaterialBindingIdx = std::uint32_t{0};

  static constexpr auto kBindings =
      std::array<vk::DescriptorSetLayoutBinding, 1>{
          layoutBinding(0, vk::DescriptorType::eStorageBuffer, 1,
                        vk::ShaderStageFlagBits::eFragment),
      };

  explicit MaterialLayout(vk::Device device)
      : DescriptorSetLayout{
            device,
            vk::DescriptorSetLayoutCreateInfo{
                vk::DescriptorSetLayoutCreateFlagBits::eUpdateAfterBindPool,
                kBindings,
            },
        } {}
};
};  // namespace vkit::vulkan::dsl
