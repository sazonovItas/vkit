#pragma once

#include "vkit/graphics/descriptor_set_layout/dsl.hpp"

namespace vkit::graphics::dsl {

struct SceneSetLayout final : public DescriptorSetLayout {
  static constexpr std::uint32_t kCameraBinding = 0;
  static constexpr std::uint32_t kEnvironmentBinding = 1;

  static constexpr auto kBindings =
      std::array<vk::DescriptorSetLayoutBinding, 2>{
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
      };

  static constexpr auto kBindingFlags =
      std::array<vk::DescriptorBindingFlags, 2>{
          vk::DescriptorBindingFlagBits::eUpdateAfterBind |
              vk::DescriptorBindingFlagBits::ePartiallyBound,
          vk::DescriptorBindingFlagBits::eUpdateAfterBind |
              vk::DescriptorBindingFlagBits::ePartiallyBound,
      };

  explicit SceneSetLayout(vk::Device device)
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
