#pragma once

#include "vulkan/descriptor_set_layout/dsl.hpp"

namespace vkit::vulkan::dsl {
struct Bindless {
  static constexpr auto kMaxBindlessResources = std::uint32_t{16536};
  static constexpr auto kMaxSamplers = std::uint32_t{32};

  static constexpr auto kTextureBinding = std::uint32_t{0};
  static constexpr auto kSamplerBinding = std::uint32_t{1};

  static constexpr auto kBindings =
      std::array<vk::DescriptorSetLayoutBinding, 2>{
          layout_binding(kTextureBinding, vk::DescriptorType::eSampledImage,
                         kMaxBindlessResources, vk::ShaderStageFlagBits::eAll),
          layout_binding(kSamplerBinding, vk::DescriptorType::eSampler,
                         kMaxSamplers, vk::ShaderStageFlagBits::eAll),
      };
};
};  // namespace vkit::vulkan::dsl
