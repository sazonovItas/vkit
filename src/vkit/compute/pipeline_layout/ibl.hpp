#pragma once

#include <array>
#include <cstdint>
#include <tuple>

#include "vkit/compute/descriptor_set_layout/ibl.hpp"
#include "vkit/graphics/pipeline_layout.hpp"

namespace vkit::compute::pl {

struct BrdfLutPipelineLayout final : graphics::PipelineLayout {
  using SetLayouts = std::tuple<const dsl::BrdfLutSetLayout&>;

  explicit BrdfLutPipelineLayout(vk::Device device, SetLayouts setLayouts)
      : BrdfLutPipelineLayout(
            device,
            std::array<vk::DescriptorSetLayout, 1>{*std::get<0>(setLayouts)}) {}

 private:
  BrdfLutPipelineLayout(vk::Device device,
                        const std::array<vk::DescriptorSetLayout, 1>& layouts)
      : PipelineLayout{device,
                       vk::PipelineLayoutCreateInfo{}.setSetLayouts(layouts)} {}
};

struct DiffusePipelineLayout final : graphics::PipelineLayout {
  using SetLayouts = std::tuple<const dsl::IblComputeSetLayout&>;

  explicit DiffusePipelineLayout(vk::Device device, SetLayouts setLayouts)
      : DiffusePipelineLayout(
            device,
            std::array<vk::DescriptorSetLayout, 1>{*std::get<0>(setLayouts)}) {}

 private:
  DiffusePipelineLayout(vk::Device device,
                        const std::array<vk::DescriptorSetLayout, 1>& layouts)
      : PipelineLayout{device,
                       vk::PipelineLayoutCreateInfo{}.setSetLayouts(layouts)} {}
};

struct SpecularPipelineLayout final : graphics::PipelineLayout {
  struct PushConstants {
    float roughness;
    std::uint32_t layer;
  };

  static constexpr auto kPushConstantRange = vk::PushConstantRange{
      vk::ShaderStageFlagBits::eCompute,
      0,
      sizeof(PushConstants),
  };

  using SetLayouts = std::tuple<const dsl::IblComputeSetLayout&>;

  explicit SpecularPipelineLayout(vk::Device device,
                                  const SetLayouts& setLayouts)
      : SpecularPipelineLayout(
            device,
            std::array<vk::DescriptorSetLayout, 1>{*std::get<0>(setLayouts)}) {}

 private:
  SpecularPipelineLayout(vk::Device device,
                         const std::array<vk::DescriptorSetLayout, 1>& layouts)
      : PipelineLayout{device, vk::PipelineLayoutCreateInfo{}
                                   .setSetLayouts(layouts)
                                   .setPushConstantRanges(kPushConstantRange)} {
  }
};

};  // namespace vkit::compute::pl
