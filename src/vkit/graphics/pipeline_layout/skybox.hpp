#pragma once

#include <array>
#include <glm/glm.hpp>
#include <tuple>

#include "vkit/graphics/descriptor_set_layout/bindless.hpp"
#include "vkit/graphics/descriptor_set_layout/scene.hpp"
#include "vkit/graphics/pipeline_layout/pl.hpp"

namespace vkit::graphics::pl {

struct SkyboxPipelineLayout final : PipelineLayout {
  struct PushConstants {
    glm::vec4 baseColor;
    float blurLevel;
  };

  static constexpr auto kPushConstantRange = vk::PushConstantRange{
      vk::ShaderStageFlagBits::eFragment,
      0,
      sizeof(PushConstants),
  };

  using SetLayouts =
      std::tuple<const dsl::SceneSetLayout&, const dsl::BindlessSetLayout&>;

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

};  // namespace vkit::graphics::pl
