#pragma once

#include "vkit/graphics/pipeline_layout.hpp"
#include "vkit/renderer/descriptor_set_layout/bindless.hpp"
#include "vkit/renderer/descriptor_set_layout/material.hpp"
#include "vkit/renderer/descriptor_set_layout/primitive.hpp"
#include "vkit/renderer/descriptor_set_layout/scene.hpp"

namespace vkit::renderer::pl {

struct PrimitiveMaterialPipelineLayout final : graphics::PipelineLayout {
  struct PushConstants {
    glm::mat4 model;
    std::uint32_t primIndex;
    std::uint32_t skinOffset;
    std::uint32_t enableSkinning;
    std::uint32_t materialType;
    std::uint32_t materialIndex;
  };

  static constexpr auto kPushConstantRange = vk::PushConstantRange{
      vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
      0,
      sizeof(PushConstants),
  };

  using SetLayouts =
      std::tuple<const dsl::SceneSetLayout&,
                 const dsl::BindlessTextureSetLayout&,
                 const dsl::MaterialSetLayout&, const dsl::PrimitiveSetLayout&>;

  explicit PrimitiveMaterialPipelineLayout(vk::Device device,
                                           SetLayouts setLayouts)
      : PrimitiveMaterialPipelineLayout(
            device, std::array<vk::DescriptorSetLayout, 4>{
                        *std::get<0>(setLayouts), *std::get<1>(setLayouts),
                        *std::get<2>(setLayouts), *std::get<3>(setLayouts)}) {}

 private:
  PrimitiveMaterialPipelineLayout(
      vk::Device device, const std::array<vk::DescriptorSetLayout, 4>& layouts)
      : PipelineLayout{device, vk::PipelineLayoutCreateInfo{}
                                   .setSetLayouts(layouts)
                                   .setPushConstantRanges(kPushConstantRange)} {
  }
};

};  // namespace vkit::renderer::pl
