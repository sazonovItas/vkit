#pragma once

#include "vulkan/descriptor_set_layout/dsl.hpp"

namespace vkit::vulkan::dsl {
struct BindlessLayout final : DescriptorSetLayout {
  static constexpr auto kMaxSamplers = std::uint32_t{32};
  static constexpr auto kMaxTextures = std::uint32_t{1024};

  static constexpr auto kSamplerBindingIdx = std::uint32_t{0};
  static constexpr auto kTexture2DBindingIdx = std::uint32_t{1};

  static constexpr auto kBindings =
      std::array<vk::DescriptorSetLayoutBinding, 2>{
          layoutBinding(kSamplerBindingIdx, vk::DescriptorType::eSampler,
                        kMaxSamplers, vk::ShaderStageFlagBits::eAllGraphics),

          layoutBinding(kTexture2DBindingIdx, vk::DescriptorType::eSampledImage,
                        kMaxTextures, vk::ShaderStageFlagBits::eAllGraphics),
      };

  static constexpr auto kBindingFlags =
      std::array<vk::DescriptorBindingFlags, 2>{
          vk::DescriptorBindingFlagBits::ePartiallyBound |
              vk::DescriptorBindingFlagBits::eUpdateAfterBind,

          vk::DescriptorBindingFlagBits::ePartiallyBound |
              vk::DescriptorBindingFlagBits::eUpdateAfterBind,
      };

  explicit BindlessLayout(vk::Device device)
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
