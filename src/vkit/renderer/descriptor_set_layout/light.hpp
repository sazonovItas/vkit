#pragma once

#include "vkit/graphics/descriptor_set_layout.hpp"

namespace vkit::renderer::dsl {

struct LightSetLayout final : public graphics::DescriptorSetLayout {
  static constexpr std::uint32_t kLightBinding = 0;

  static constexpr auto kBindings =
      std::array<vk::DescriptorSetLayoutBinding, 1>{
          vk::DescriptorSetLayoutBinding{
              kLightBinding,
              vk::DescriptorType::eUniformBuffer,
              1,
              vk::ShaderStageFlagBits::eVertex,
          },
      };

  static constexpr auto kBindingFlags =
      std::array<vk::DescriptorBindingFlags, 1>{
          vk::DescriptorBindingFlagBits::ePartiallyBound,
      };

  explicit LightSetLayout(vk::Device device)
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
