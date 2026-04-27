#pragma once

#include <array>
#include <cstdint>
#include <tuple>

#include "vkit/graphics/descriptor_set_layout/bindless.hpp"
#include "vkit/graphics/descriptor_set_layout/material.hpp"
#include "vkit/graphics/descriptor_set_layout/primitive.hpp"
#include "vkit/graphics/descriptor_set_layout/scene.hpp"
#include "vkit/graphics/pipeline_layout/pl.hpp"

namespace vkit::graphics::pl {

struct PrimitivePipelineLayout final : PipelineLayout {
  struct PushConstants {
    glm::mat4 model;
    std::uint32_t primIndex;
    std::uint32_t skinOffset;
    std::uint32_t materialType;
    std::uint32_t materialIndex;
  };

  static constexpr auto kPushConstantRange = vk::PushConstantRange{
      vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
      0,
      sizeof(PushConstants),
  };

  using SetLayouts =
      std::tuple<const dsl::SceneSetLayout&, const dsl::BindlessSetLayout&,
                 const dsl::MaterialSetLayout&, const dsl::PrimitiveSetLayout&>;

  explicit PrimitivePipelineLayout(vk::Device device, SetLayouts setLayouts)
      : PipelineLayout{device, createPipelineCreateInfo(setLayouts)} {}

 private:
  static auto createPipelineCreateInfo(SetLayouts setLayouts)
      -> vk::PipelineLayoutCreateInfo {
    const auto set_layouts = std::array<vk::DescriptorSetLayout, 4>{
        *std::get<0>(setLayouts),
        *std::get<1>(setLayouts),
        *std::get<2>(setLayouts),
        *std::get<3>(setLayouts),
    };

    return vk::PipelineLayoutCreateInfo{{}, set_layouts, kPushConstantRange};
  }
};

};  // namespace vkit::graphics::pl
