#pragma once

#include <array>

#include "vkit/graphics/descriptor_set_layout.hpp"

namespace vkit::compute::dsl {

struct NoiseSetLayout final : public graphics::DescriptorSetLayout {
  static constexpr std::uint32_t kOutImageF32Binding = 0;
  static constexpr std::uint32_t kOutImageUnormBinding = 1;

  static constexpr auto kBindings =
      std::array<vk::DescriptorSetLayoutBinding, 2>{
          vk::DescriptorSetLayoutBinding{
              kOutImageF32Binding,
              vk::DescriptorType::eStorageImage,
              1,
              vk::ShaderStageFlagBits::eCompute,
          },
          vk::DescriptorSetLayoutBinding{
              kOutImageUnormBinding,
              vk::DescriptorType::eStorageImage,
              1,
              vk::ShaderStageFlagBits::eCompute,
          },
      };

  explicit NoiseSetLayout(vk::Device device)
      : DescriptorSetLayout{
            device,
            vk::StructureChain{
                vk::DescriptorSetLayoutCreateInfo{
                    vk::DescriptorSetLayoutCreateFlags{},
                    kBindings,
                },
            }
                .get(),
        } {}
};

};  // namespace vkit::compute::dsl
