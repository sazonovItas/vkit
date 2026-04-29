#pragma once

#include "vkit/graphics/descriptor_set_layout.hpp"

namespace vkit::renderer::dsl {

struct BrdfLutSetLayout final : public graphics::DescriptorSetLayout {
  static constexpr std::uint32_t kOutImageBinding = 0;

  static constexpr auto kBindings =
      std::array<vk::DescriptorSetLayoutBinding, 1>{
          vk::DescriptorSetLayoutBinding{
              kOutImageBinding,
              vk::DescriptorType::eStorageImage,
              1,
              vk::ShaderStageFlagBits::eCompute,
          },
      };

  explicit BrdfLutSetLayout(vk::Device device)
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

struct IblComputeSetLayout final : public graphics::DescriptorSetLayout {
  static constexpr std::uint32_t kSourceEnvMapBinding = 0;
  static constexpr std::uint32_t kOutImageBinding = 1;

  static constexpr auto kBindings =
      std::array<vk::DescriptorSetLayoutBinding, 2>{
          vk::DescriptorSetLayoutBinding{
              kSourceEnvMapBinding,
              vk::DescriptorType::eCombinedImageSampler,
              1,
              vk::ShaderStageFlagBits::eCompute,
          },
          vk::DescriptorSetLayoutBinding{
              kOutImageBinding,
              vk::DescriptorType::eStorageImage,
              1,
              vk::ShaderStageFlagBits::eCompute,
          },
      };

  explicit IblComputeSetLayout(vk::Device device)
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

};  // namespace vkit::renderer::dsl
