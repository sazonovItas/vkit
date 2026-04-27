#pragma once

#include "vkit/graphics/descriptor_set_layout/dsl.hpp"

namespace vkit::graphics::dsl {

struct BindlessSetLayout final : DescriptorSetLayout {
  static constexpr auto kMaxSamplers = std::uint32_t{32};
  static constexpr auto kMaxTextures = std::uint32_t{1024};
  static constexpr auto kMaxImages = std::uint32_t{1024};

  static constexpr auto kSamplerBinding = std::uint32_t{0};
  static constexpr auto kTexture2DBinding = std::uint32_t{1};
  static constexpr auto kImage2DBinding = std::uint32_t{2};

  static constexpr auto kBindings =
      std::array<vk::DescriptorSetLayoutBinding, 3>{
          vk::DescriptorSetLayoutBinding{
              kSamplerBinding,
              vk::DescriptorType::eSampler,
              kMaxSamplers,
              vk::ShaderStageFlagBits::eAllGraphics,
          },
          vk::DescriptorSetLayoutBinding{
              kTexture2DBinding,
              vk::DescriptorType::eSampledImage,
              kMaxTextures,
              vk::ShaderStageFlagBits::eAllGraphics,
          },
          vk::DescriptorSetLayoutBinding{
              kImage2DBinding,
              vk::DescriptorType::eStorageImage,
              kMaxImages,
              vk::ShaderStageFlagBits::eAllGraphics,
          },
      };

  static constexpr auto kBindingFlags =
      std::array<vk::DescriptorBindingFlags, 3>{
          vk::DescriptorBindingFlagBits::ePartiallyBound |
              vk::DescriptorBindingFlagBits::eUpdateAfterBind,
          vk::DescriptorBindingFlagBits::ePartiallyBound |
              vk::DescriptorBindingFlagBits::eUpdateAfterBind,
          vk::DescriptorBindingFlagBits::ePartiallyBound |
              vk::DescriptorBindingFlagBits::eUpdateAfterBind,
      };

  explicit BindlessSetLayout(vk::Device device)
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

};  // namespace vkit::graphics::dsl
