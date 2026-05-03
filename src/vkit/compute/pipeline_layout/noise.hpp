#pragma once

#include <array>
#include <cstdint>
#include <tuple>

#include "vkit/compute/descriptor_set_layout/noise.hpp"
#include "vkit/core/events/noise.hpp"
#include "vkit/graphics/pipeline_layout.hpp"

namespace vkit::compute::pl {

struct NoisePipelineLayout final : graphics::PipelineLayout {
  using PushConstants = core::events::NoisePushConstants;

  static constexpr auto kPushConstantRange = vk::PushConstantRange{
      vk::ShaderStageFlagBits::eCompute,
      0,
      sizeof(PushConstants),
  };

  using SetLayouts = std::tuple<const dsl::NoiseSetLayout&>;

  explicit NoisePipelineLayout(vk::Device device, const SetLayouts& setLayouts)
      : PipelineLayout{device, createPipelineCreateInfo(setLayouts)} {}

 private:
  static auto createPipelineCreateInfo(const SetLayouts& setLayouts)
      -> vk::PipelineLayoutCreateInfo {
    const auto set_layouts = std::array<vk::DescriptorSetLayout, 1>{
        *std::get<0>(setLayouts),
    };

    return vk::PipelineLayoutCreateInfo{{}, set_layouts, kPushConstantRange};
  }
};

};  // namespace vkit::compute::pl
