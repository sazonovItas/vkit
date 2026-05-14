#pragma once

#include <array>
#include <tuple>

#include "vkit/graphics/pipeline_layout.hpp"
#include "vkit/renderer/descriptor_set_layout/bindless.hpp"
#include "vkit/renderer/descriptor_set_layout/material.hpp"
#include "vkit/renderer/descriptor_set_layout/scene.hpp"

namespace vkit::renderer::pl {

struct RaySphereMaterialPipelineLayout final : graphics::PipelineLayout {
  struct PushConstants {
    glm::mat4 model;
    std::uint32_t materialType;
    std::uint32_t materialIndex;
    std::uint32_t enableDepthWrite;
  };

  static constexpr auto kPushConstantRange = vk::PushConstantRange{
      vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
      0,
      sizeof(PushConstants),
  };

  using SetLayouts = std::tuple<const dsl::SceneSetLayout&,
                                const dsl::BindlessTextureSetLayout&,
                                const dsl::MaterialSetLayout&>;

  explicit RaySphereMaterialPipelineLayout(vk::Device device,
                                           SetLayouts setLayouts)
      : RaySphereMaterialPipelineLayout(device,
                                        std::array<vk::DescriptorSetLayout, 3>{
                                            *std::get<0>(setLayouts),
                                            *std::get<1>(setLayouts),
                                            *std::get<2>(setLayouts),
                                        }) {}

 private:
  RaySphereMaterialPipelineLayout(
      vk::Device device, const std::array<vk::DescriptorSetLayout, 3>& layouts)
      : PipelineLayout{device, vk::PipelineLayoutCreateInfo{}
                                   .setSetLayouts(layouts)
                                   .setPushConstantRanges(kPushConstantRange)} {
  }
};

};  // namespace vkit::renderer::pl
