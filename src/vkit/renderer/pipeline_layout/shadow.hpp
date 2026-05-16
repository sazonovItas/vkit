#pragma once

#include <array>
#include <tuple>

#include "vkit/graphics/pipeline_layout.hpp"
#include "vkit/renderer/descriptor_set_layout/light.hpp"
#include "vkit/renderer/descriptor_set_layout/primitive.hpp"

namespace vkit::renderer::pl {

struct ShadowPipelineLayout final : graphics::PipelineLayout {
  struct PushConstants {
    glm::mat4 model;
    std::uint32_t primIndex;
    std::uint32_t skinOffset;
    std::uint32_t enableSkinning;
    std::uint32_t _pad;
  };

  static constexpr auto kPushConstantRange = vk::PushConstantRange{
      vk::ShaderStageFlagBits::eVertex,
      0,
      sizeof(PushConstants),
  };

  using SetLayouts = std::tuple<const dsl::LightSetLayout&,
                                const dsl::PrimitiveSetLayout&>;

  explicit ShadowPipelineLayout(vk::Device device, SetLayouts setLayouts)
      : ShadowPipelineLayout(
            device, std::array<vk::DescriptorSetLayout, 2>{
                        *std::get<0>(setLayouts), *std::get<1>(setLayouts)}) {}

 private:
  ShadowPipelineLayout(vk::Device device,
                       const std::array<vk::DescriptorSetLayout, 2>& layouts)
      : PipelineLayout{device, vk::PipelineLayoutCreateInfo{}
                                   .setSetLayouts(layouts)
                                   .setPushConstantRanges(kPushConstantRange)} {
  }
};

};  // namespace vkit::renderer::pl
