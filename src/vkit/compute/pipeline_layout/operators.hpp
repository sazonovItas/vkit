#pragma once

#include <array>
#include <tuple>

#include "vkit/compute/descriptor_set_layout/image_operator.hpp"
#include "vkit/core/events/operators.hpp"
#include "vkit/graphics/pipeline_layout.hpp"

namespace vkit::compute::pl {

struct SobelPipelineLayout final : graphics::PipelineLayout {
  static constexpr auto kPushConstantRange =
      vk::PushConstantRange{vk::ShaderStageFlagBits::eCompute, 0,
                            sizeof(core::events::SobelPushConstants)};
  using SetLayouts = std::tuple<const dsl::ImageOperatorSetLayout&>;

  explicit SobelPipelineLayout(vk::Device device, const SetLayouts& setLayouts)
      : PipelineLayout{device, createPipelineCreateInfo(setLayouts)} {}

 private:
  static auto createPipelineCreateInfo(const SetLayouts& setLayouts)
      -> vk::PipelineLayoutCreateInfo {
    const auto set_layouts =
        std::array<vk::DescriptorSetLayout, 1>{*std::get<0>(setLayouts)};
    return vk::PipelineLayoutCreateInfo{{}, set_layouts, kPushConstantRange};
  }
};

struct HeightMapPipelineLayout final : graphics::PipelineLayout {
  static constexpr auto kPushConstantRange =
      vk::PushConstantRange{vk::ShaderStageFlagBits::eCompute, 0,
                            sizeof(core::events::HeightMapPushConstants)};
  using SetLayouts = std::tuple<const dsl::ImageOperatorSetLayout&>;

  explicit HeightMapPipelineLayout(vk::Device device,
                                   const SetLayouts& setLayouts)
      : PipelineLayout{device, createPipelineCreateInfo(setLayouts)} {}

 private:
  static auto createPipelineCreateInfo(const SetLayouts& setLayouts)
      -> vk::PipelineLayoutCreateInfo {
    const auto set_layouts =
        std::array<vk::DescriptorSetLayout, 1>{*std::get<0>(setLayouts)};
    return vk::PipelineLayoutCreateInfo{{}, set_layouts, kPushConstantRange};
  }
};

struct TintPipelineLayout final : graphics::PipelineLayout {
  static constexpr auto kPushConstantRange =
      vk::PushConstantRange{vk::ShaderStageFlagBits::eCompute, 0,
                            sizeof(core::events::TintPushConstants)};
  using SetLayouts = std::tuple<const dsl::ImageOperatorSetLayout&>;

  explicit TintPipelineLayout(vk::Device device, const SetLayouts& setLayouts)
      : PipelineLayout{device, createPipelineCreateInfo(setLayouts)} {}

 private:
  static auto createPipelineCreateInfo(const SetLayouts& setLayouts)
      -> vk::PipelineLayoutCreateInfo {
    const auto set_layouts =
        std::array<vk::DescriptorSetLayout, 1>{*std::get<0>(setLayouts)};
    return vk::PipelineLayoutCreateInfo{{}, set_layouts, kPushConstantRange};
  }
};

};  // namespace vkit::compute::pl
