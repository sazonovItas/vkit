#pragma once

#include "vulkan/descriptor_set_layout/bindless.hpp"
#include "vulkan/descriptor_set_layout/scene.hpp"
#include "vulkan/pipeline_layout/pl.hpp"

namespace vkit::vulkan::pl {
struct SkyboxLayout final : PipelineLayout {
  struct PushConstants {
    glm::vec4 envBaseColor;
    std::int32_t envMapIdx;
  };

  static constexpr auto kPushConstantRange = vk::PushConstantRange{
      vk::ShaderStageFlagBits::eFragment,
      0,
      sizeof(PushConstants),
  };

  using DescriptorSetLayouts =
      std::tuple<const dsl::SceneLayout&, const dsl::BindlessLayout&>;

  explicit SkyboxLayout(vk::Device device, DescriptorSetLayouts setLayouts)
      : PipelineLayout{
            device,
            createPipelineCreateInfo(setLayouts),
        } {}

 private:
  static auto createPipelineCreateInfo(DescriptorSetLayouts setLayouts)
      -> vk::PipelineLayoutCreateInfo {
    const auto set_layouts = std::array<vk::DescriptorSetLayout, 2>{
        *get<0>(setLayouts),
        *get<1>(setLayouts),
    };

    return vk::PipelineLayoutCreateInfo{{}, set_layouts, kPushConstantRange};
  }
};
};  // namespace vkit::vulkan::pl
