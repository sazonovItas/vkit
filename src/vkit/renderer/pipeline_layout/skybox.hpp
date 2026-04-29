#pragma once

#include <array>
#include <glm/glm.hpp>
#include <tuple>

#include "vkit/graphics/pipeline_layout.hpp"
#include "vkit/renderer/descriptor_set_layout/bindless.hpp"
#include "vkit/renderer/descriptor_set_layout/scene.hpp"

namespace vkit::renderer::pl {

struct SkyboxPipelineLayout final : graphics::PipelineLayout {
  struct PushConstants {
    glm::vec4 baseColor;
    float blurLevel;
  };

  static constexpr auto kPushConstantRange = vk::PushConstantRange{
      vk::ShaderStageFlagBits::eFragment,
      0,
      sizeof(PushConstants),
  };

  using SetLayouts = std::tuple<const dsl::SceneSetLayout&,
                                const dsl::BindlessTextureSetLayout&>;

  explicit SkyboxPipelineLayout(vk::Device device, SetLayouts setLayouts)
      : PipelineLayout{device, createPipelineCreateInfo(setLayouts)} {}

 private:
  static auto createPipelineCreateInfo(SetLayouts setLayouts)
      -> vk::PipelineLayoutCreateInfo {
    const auto set_layouts = std::array<vk::DescriptorSetLayout, 2>{
        *std::get<0>(setLayouts),
        *std::get<1>(setLayouts),
    };

    return vk::PipelineLayoutCreateInfo{{}, set_layouts, kPushConstantRange};
  }
};

};  // namespace vkit::renderer::pl
