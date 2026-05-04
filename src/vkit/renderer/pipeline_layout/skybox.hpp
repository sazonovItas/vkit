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
      : SkyboxPipelineLayout(
            device, std::array<vk::DescriptorSetLayout, 2>{
                        *std::get<0>(setLayouts), *std::get<1>(setLayouts)}) {}

 private:
  SkyboxPipelineLayout(vk::Device device,
                       const std::array<vk::DescriptorSetLayout, 2>& layouts)
      : PipelineLayout{device, vk::PipelineLayoutCreateInfo{}
                                   .setSetLayouts(layouts)
                                   .setPushConstantRanges(kPushConstantRange)} {
  }
};

};  // namespace vkit::renderer::pl
