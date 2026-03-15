#pragma once

#include <array>
#include <tuple>

#include "vulkan/descriptor_set_layout/bindless.hpp"
#include "vulkan/pipeline_layout/pl.hpp"

namespace vkit::vulkan::pl {
struct PrefilterSpecularIBLLayout final : PipelineLayout {
  struct PushConstants {
    std::uint32_t sourceEnvMapIdx;
    std::uint32_t outPrefilterMapIdx;
    float roughness;
  };

  static constexpr auto kPushConstantRange = vk::PushConstantRange{
      vk::ShaderStageFlagBits::eCompute,
      0,
      sizeof(PushConstants),
  };

  using DescriptorSetLayouts = std::tuple<const dsl::BindlessLayout&>;

  explicit PrefilterSpecularIBLLayout(vk::Device device,
                                      DescriptorSetLayouts setLayouts)
      : PipelineLayout{
            device,
            createPipelineCreateInfo(setLayouts),
        } {}

 private:
  static auto createPipelineCreateInfo(DescriptorSetLayouts setLayouts)
      -> vk::PipelineLayoutCreateInfo {
    const auto set_layouts = std::array<vk::DescriptorSetLayout, 1>{
        *std::get<0>(setLayouts),
    };

    return vk::PipelineLayoutCreateInfo{{}, set_layouts, kPushConstantRange};
  }
};
};  // namespace vkit::vulkan::pl
