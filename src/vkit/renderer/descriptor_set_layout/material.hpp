#pragma once

#include "vkit/graphics/descriptor_set_layout.hpp"

namespace vkit::renderer::dsl {

struct MaterialSetLayout final : public graphics::DescriptorSetLayout {
  static constexpr std::uint32_t kDiffuseBinding = 0;
  static constexpr std::uint32_t kDiffuseSpecularBinding = 1;
  static constexpr std::uint32_t kPrincipledBsdfBinding = 2;

  static constexpr auto kBindings =
      std::array<vk::DescriptorSetLayoutBinding, 3>{
          vk::DescriptorSetLayoutBinding{
              kDiffuseBinding,
              vk::DescriptorType::eStorageBuffer,
              1,
              vk::ShaderStageFlagBits::eFragment,
          },
          vk::DescriptorSetLayoutBinding{
              kDiffuseSpecularBinding,
              vk::DescriptorType::eStorageBuffer,
              1,
              vk::ShaderStageFlagBits::eFragment,
          },
          vk::DescriptorSetLayoutBinding{
              kPrincipledBsdfBinding,
              vk::DescriptorType::eStorageBuffer,
              1,
              vk::ShaderStageFlagBits::eFragment,
          },
      };

  static constexpr auto kBindingFlags =
      std::array<vk::DescriptorBindingFlags, 3>{
          vk::DescriptorBindingFlagBits::eUpdateAfterBind |
              vk::DescriptorBindingFlagBits::ePartiallyBound,
          vk::DescriptorBindingFlagBits::eUpdateAfterBind |
              vk::DescriptorBindingFlagBits::ePartiallyBound,
          vk::DescriptorBindingFlagBits::eUpdateAfterBind |
              vk::DescriptorBindingFlagBits::ePartiallyBound,
      };

  explicit MaterialSetLayout(vk::Device device)
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

};  // namespace vkit::renderer::dsl
