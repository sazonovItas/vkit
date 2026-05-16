#pragma once

#include <array>
#include <tuple>

#include "vkit/graphics/pipeline_layout.hpp"
#include "vkit/renderer/descriptor_set_layout/scene.hpp"

namespace vkit::renderer::pl {

struct LightGizmoPipelineLayout final : graphics::PipelineLayout {
  struct PushConstants {
    float size;
  };

  static constexpr auto kPushConstantRange = vk::PushConstantRange{
      vk::ShaderStageFlagBits::eVertex,
      0,
      sizeof(PushConstants),
  };

  using SetLayouts = std::tuple<const dsl::SceneSetLayout&>;

  explicit LightGizmoPipelineLayout(vk::Device device, SetLayouts setLayouts)
      : LightGizmoPipelineLayout(
            device, std::array<vk::DescriptorSetLayout, 1>{
                        *std::get<0>(setLayouts)}) {}

 private:
  LightGizmoPipelineLayout(vk::Device device,
                            const std::array<vk::DescriptorSetLayout, 1>& layouts)
      : PipelineLayout{device, vk::PipelineLayoutCreateInfo{}
                                   .setSetLayouts(layouts)
                                   .setPushConstantRanges(kPushConstantRange)} {
  }
};

};  // namespace vkit::renderer::pl
