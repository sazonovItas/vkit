#pragma once

#include "vulkan/descriptor_set_layout/dsl.hpp"

namespace vkit::vulkan::dsl {
struct Bindless : public DescriptorSetLayout {
  static constexpr auto kMaxBindlessResources = std::uint32_t{16536};
  static constexpr auto kMaxSamplers = std::uint32_t{32};

  static constexpr auto kTexture2DBinding = std::uint32_t{0};
  static constexpr auto kTextureCubeBinding = std::uint32_t{1};
  static constexpr auto kSamplerBinding = std::uint32_t{2};

  static constexpr auto kBindings =
      std::array<vk::DescriptorSetLayoutBinding, 3>{
          layoutBinding(0, vk::DescriptorType::eSampledImage,
                        kMaxBindlessResources, vk::ShaderStageFlagBits::eAll),

          layoutBinding(1, vk::DescriptorType::eSampledImage,
                        kMaxBindlessResources, vk::ShaderStageFlagBits::eAll),

          layoutBinding(2, vk::DescriptorType::eSampler, kMaxSamplers,
                        vk::ShaderStageFlagBits::eAll),
      };

  static constexpr auto kBindingFlags =
      std::array<vk::DescriptorBindingFlags, 3>{
          vk::DescriptorBindingFlagBits::ePartiallyBound |
              vk::DescriptorBindingFlagBits::eUpdateAfterBind |
              vk::DescriptorBindingFlagBits::eVariableDescriptorCount,

          vk::DescriptorBindingFlagBits::ePartiallyBound |
              vk::DescriptorBindingFlagBits::eUpdateAfterBind |
              vk::DescriptorBindingFlagBits::eVariableDescriptorCount,

          vk::DescriptorBindingFlagBits::ePartiallyBound,
      };

  explicit Bindless(vk::Device device)
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
