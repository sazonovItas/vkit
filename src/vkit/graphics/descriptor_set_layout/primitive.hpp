#pragma once

#include "vkit/graphics/descriptor_set_layout/dsl.hpp"

namespace vkit::graphics::dsl {

struct PrimitiveSetLayout final : public DescriptorSetLayout {
  static constexpr std::uint32_t kPrimitiveBinding = 0;
  static constexpr std::uint32_t kJointBinding = 1;

  static constexpr auto kBindings =
      std::array<vk::DescriptorSetLayoutBinding, 2>{
          vk::DescriptorSetLayoutBinding{
              kPrimitiveBinding,
              vk::DescriptorType::eStorageBuffer,
              1,
              vk::ShaderStageFlagBits::eVertex |
                  vk::ShaderStageFlagBits::eFragment,
          },
          vk::DescriptorSetLayoutBinding{
              kJointBinding,
              vk::DescriptorType::eStorageBuffer,
              1,
              vk::ShaderStageFlagBits::eVertex,
          },
      };

  static constexpr auto kBindingFlags =
      std::array<vk::DescriptorBindingFlags, 2>{
          vk::DescriptorBindingFlagBits::eUpdateAfterBind |
              vk::DescriptorBindingFlagBits::ePartiallyBound,
          vk::DescriptorBindingFlagBits::eUpdateAfterBind |
              vk::DescriptorBindingFlagBits::ePartiallyBound,
      };

  explicit PrimitiveSetLayout(vk::Device device)
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
