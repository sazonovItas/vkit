#pragma once

#include "vkit/graphics/descriptor_set_layout.hpp"

namespace vkit::renderer::dsl {

struct SceneSetLayout final : public graphics::DescriptorSetLayout {
  static constexpr std::uint32_t kCameraBinding = 0;
  static constexpr std::uint32_t kEnvironmentBinding = 1;
  static constexpr std::uint32_t kSceneParamsBinding = 2;
  static constexpr std::uint32_t kLightBinding = 3;
  static constexpr std::uint32_t kShadowMapBinding = 4;

  static constexpr auto kBindings =
      std::array<vk::DescriptorSetLayoutBinding, 5>{
          vk::DescriptorSetLayoutBinding{
              kCameraBinding,
              vk::DescriptorType::eUniformBuffer,
              1,
              vk::ShaderStageFlagBits::eAllGraphics,
          },
          vk::DescriptorSetLayoutBinding{
              kEnvironmentBinding,
              vk::DescriptorType::eUniformBuffer,
              1,
              vk::ShaderStageFlagBits::eAllGraphics,
          },
          vk::DescriptorSetLayoutBinding{
              kSceneParamsBinding,
              vk::DescriptorType::eUniformBuffer,
              1,
              vk::ShaderStageFlagBits::eAllGraphics,
          },
          vk::DescriptorSetLayoutBinding{
              kLightBinding,
              vk::DescriptorType::eStorageBuffer,
              1,
              vk::ShaderStageFlagBits::eAllGraphics,
          },
          vk::DescriptorSetLayoutBinding{
              kShadowMapBinding,
              vk::DescriptorType::eCombinedImageSampler,
              1,
              vk::ShaderStageFlagBits::eFragment,
          },
      };

  static constexpr auto kBindingFlags =
      std::array<vk::DescriptorBindingFlags, 5>{
          vk::DescriptorBindingFlagBits::ePartiallyBound,
          vk::DescriptorBindingFlagBits::ePartiallyBound,
          vk::DescriptorBindingFlagBits::ePartiallyBound,
          vk::DescriptorBindingFlagBits::ePartiallyBound,
          vk::DescriptorBindingFlagBits::ePartiallyBound,
      };

  explicit SceneSetLayout(vk::Device device)
      : DescriptorSetLayout{
            device,
            vk::StructureChain{
                vk::DescriptorSetLayoutCreateInfo{
                    {},
                    kBindings,
                },
                vk::DescriptorSetLayoutBindingFlagsCreateInfo{
                    kBindingFlags,
                },
            }
                .get(),
        } {}
};

};  // namespace vkit::renderer::dsl
