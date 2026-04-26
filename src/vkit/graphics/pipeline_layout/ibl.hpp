#pragma once

#include <array>
#include <cstdint>
#include <tuple>

#include "vkit/graphics/descriptor_set_layout/ibl.hpp"
#include "vkit/graphics/pipeline_layout/pl.hpp"

namespace vkit::graphics::pl {

struct BrdfLutPipelineLayout final : PipelineLayout {
  using SetLayouts = std::tuple<const dsl::BrdfLutSetLayout&>;

  explicit BrdfLutPipelineLayout(vk::Device device, SetLayouts setLayouts)
      : PipelineLayout{device, createPipelineCreateInfo(setLayouts)} {}

 private:
  static auto createPipelineCreateInfo(SetLayouts setLayouts)
      -> vk::PipelineLayoutCreateInfo {
    const auto set_layouts = std::array<vk::DescriptorSetLayout, 1>{
        *std::get<0>(setLayouts),
    };

    return vk::PipelineLayoutCreateInfo{{}, set_layouts, {}};
  }
};

struct DiffusePipelineLayout final : PipelineLayout {
  using SetLayouts = std::tuple<const dsl::IblComputeSetLayout&>;

  explicit DiffusePipelineLayout(vk::Device device, SetLayouts setLayouts)
      : PipelineLayout{device, createPipelineCreateInfo(setLayouts)} {}

 private:
  static auto createPipelineCreateInfo(SetLayouts setLayouts)
      -> vk::PipelineLayoutCreateInfo {
    const auto set_layouts = std::array<vk::DescriptorSetLayout, 1>{
        *std::get<0>(setLayouts),
    };

    return vk::PipelineLayoutCreateInfo{{}, set_layouts, {}};
  }
};

struct SpecularPipelineLayout final : PipelineLayout {
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

};  // namespace vkit::graphics::pl
