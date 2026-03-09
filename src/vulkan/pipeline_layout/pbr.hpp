#pragma once

#include "vulkan/descriptor_set_layout/bindless.hpp"
#include "vulkan/descriptor_set_layout/material.hpp"
#include "vulkan/descriptor_set_layout/scene.hpp"
#include "vulkan/pipeline_layout/pl.hpp"

namespace vkit::vulkan::pl {
struct PBRPipelineLayout final : PipelineLayout {
  struct PushConstants {
    std::uint32_t meshIdx;
    std::uint32_t materialIdx;
    vk::DeviceAddress vertices;
    glm::mat4 transform;
  };

  static constexpr auto kPushConstantRange = vk::PushConstantRange{
      vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
      0,
      sizeof(PushConstants),
  };

  using DescriptorSetLayouts =
      std::tuple<const dsl::SceneLayout&, const dsl::MaterialLayout&,
                 const dsl::BindlessLayout&>;

  explicit PBRPipelineLayout(vk::Device device, DescriptorSetLayouts setLayouts)
      : PipelineLayout{
            device,
            createPipelineCreateInfo(setLayouts),
        } {}

 private:
  static auto createPipelineCreateInfo(DescriptorSetLayouts setLayouts)
      -> vk::PipelineLayoutCreateInfo {
    const auto set_layouts = std::array<vk::DescriptorSetLayout, 3>{
        *get<0>(setLayouts),
        *get<1>(setLayouts),
        *get<2>(setLayouts),
    };

    return vk::PipelineLayoutCreateInfo{{}, set_layouts, kPushConstantRange};
  }
};
};  // namespace vkit::vulkan::pl
