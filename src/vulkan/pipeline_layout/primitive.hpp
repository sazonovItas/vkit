#pragma once

#include "vulkan/descriptor_set_layout/bindless.hpp"
#include "vulkan/descriptor_set_layout/material.hpp"
#include "vulkan/descriptor_set_layout/scene.hpp"
#include "vulkan/pipeline_layout/pl.hpp"

namespace vkit::vulkan::pl {
struct Primitive final : PipelineLayout {
  struct PushConstants {
    std::int32_t materialIdx;
    vk::DeviceAddress vertices;
  };

  static constexpr auto kRange = vk::PushConstantRange{
      vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
      0,
      sizeof(PushConstants),
  };

  explicit Primitive(
      vk::Device device,
      std::tuple<const dsl::Scene&, const dsl::Bindless&, const dsl::Material&>
          setLayouts)
      : PipelineLayout{
            device,
            createPipelineCreateInfo(setLayouts),
        } {}

 private:
  static auto createPipelineCreateInfo(
      std::tuple<const dsl::Scene&, const dsl::Bindless&, const dsl::Material&>
          setLayouts) -> vk::PipelineLayoutCreateInfo {
    const auto set_layouts = std::array<vk::DescriptorSetLayout, 3>{
        *get<0>(setLayouts),
        *get<1>(setLayouts),
        *get<2>(setLayouts),
    };

    return vk::PipelineLayoutCreateInfo{{}, set_layouts, kRange};
  }
};
};  // namespace vkit::vulkan::pl
