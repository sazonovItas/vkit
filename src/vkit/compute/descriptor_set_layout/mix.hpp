#pragma once

#include <array>

#include "vkit/graphics/descriptor_set_layout.hpp"

namespace vkit::compute::dsl {

struct MixOperatorSetLayout final : public graphics::DescriptorSetLayout {
  static constexpr auto kBindings =
      std::array<vk::DescriptorSetLayoutBinding, 5>{
          vk::DescriptorSetLayoutBinding{
              0,
              vk::DescriptorType::eCombinedImageSampler,
              1,
              vk::ShaderStageFlagBits::eCompute,
          },
          vk::DescriptorSetLayoutBinding{
              1,
              vk::DescriptorType::eCombinedImageSampler,
              1,
              vk::ShaderStageFlagBits::eCompute,
          },
          vk::DescriptorSetLayoutBinding{
              2,
              vk::DescriptorType::eCombinedImageSampler,
              1,
              vk::ShaderStageFlagBits::eCompute,
          },
          vk::DescriptorSetLayoutBinding{
              3,
              vk::DescriptorType::eStorageImage,
              1,
              vk::ShaderStageFlagBits::eCompute,
          },
          vk::DescriptorSetLayoutBinding{
              4,
              vk::DescriptorType::eStorageImage,
              1,
              vk::ShaderStageFlagBits::eCompute,
          },
      };

  explicit MixOperatorSetLayout(vk::Device device)
      : DescriptorSetLayout{
            device,
            vk::StructureChain{vk::DescriptorSetLayoutCreateInfo{{}, kBindings}}
                .get()} {}
};

};  // namespace vkit::compute::dsl
