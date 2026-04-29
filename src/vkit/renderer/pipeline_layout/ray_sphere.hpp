#pragma once

#include <array>
#include <cstdint>
#include <tuple>

#include "vkit/graphics/pipeline_layout.hpp"
#include "vkit/renderer/descriptor_set_layout/bindless.hpp"
#include "vkit/renderer/descriptor_set_layout/material.hpp"
#include "vkit/renderer/descriptor_set_layout/scene.hpp"

namespace vkit::renderer::pl {

struct RaySpherePipelineLayout final : graphics::PipelineLayout {
  struct PushConstants {
    glm::mat4 model;
    std::uint32_t materialType;
    std::uint32_t materialIndex;
  };

  static constexpr auto kPushConstantRange = vk::PushConstantRange{
      vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
      0,
      sizeof(PushConstants),
  };

  using SetLayouts = std::tuple<const dsl::SceneSetLayout&,
                                const dsl::BindlessTextureSetLayout&,
                                const dsl::MaterialSetLayout&>;

  explicit RaySpherePipelineLayout(vk::Device device, SetLayouts setLayouts)
      : PipelineLayout{device, createPipelineCreateInfo(setLayouts)} {}

 private:
  static auto createPipelineCreateInfo(SetLayouts setLayouts)
      -> vk::PipelineLayoutCreateInfo {
    const auto set_layouts = std::array<vk::DescriptorSetLayout, 3>{
        *std::get<0>(setLayouts),
        *std::get<1>(setLayouts),
        *std::get<2>(setLayouts),
    };

    return vk::PipelineLayoutCreateInfo{{}, set_layouts, kPushConstantRange};
  }
};

};  // namespace vkit::renderer::pl
