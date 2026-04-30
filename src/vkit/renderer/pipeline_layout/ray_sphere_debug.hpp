#pragma once

#include <array>
#include <tuple>

#include "vkit/graphics/pipeline_layout.hpp"
#include "vkit/renderer/descriptor_set_layout/scene.hpp"

namespace vkit::renderer::pl {

struct RaySphereDebugPipelineLayout final : graphics::PipelineLayout {
  struct PushConstants {
    glm::mat4 model;
    std::uint32_t materialIdx;
  };

  static constexpr auto kPushConstantRange = vk::PushConstantRange{
      vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
      0,
      sizeof(PushConstants),
  };

  using SetLayouts = std::tuple<const dsl::SceneSetLayout&>;

  explicit RaySphereDebugPipelineLayout(vk::Device device,
                                        SetLayouts setLayouts)
      : PipelineLayout{device, createPipelineCreateInfo(setLayouts)} {}

 private:
  static auto createPipelineCreateInfo(SetLayouts setLayouts)
      -> vk::PipelineLayoutCreateInfo {
    const auto set_layouts = std::array<vk::DescriptorSetLayout, 1>{
        *std::get<0>(setLayouts),
    };

    return vk::PipelineLayoutCreateInfo{{}, set_layouts, kPushConstantRange};
  }
};

};  // namespace vkit::renderer::pl
