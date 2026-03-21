#pragma once

#include <array>
#include <tuple>

#include "vulkan/descriptor_set_layout/bindless.hpp"
#include "vulkan/pipeline_layout/pl.hpp"

namespace vkit::vulkan::pl {
struct ProceduralTextureLayout final : PipelineLayout {
  struct PushConstants {
    uint32_t outImageIdx;
    uint32_t outNormalImageIdx;
    uint32_t generateColorMap;
    uint32_t generateNormalMap;
    glm::vec2 tileSize;     // e.g., {128.0f, 64.0f}
    uint32_t patternType;   // 0 for Grid, 1 for Bricks
    float mortarThickness;  // e.g., 4.0f
    glm::vec4 brickColor;   // e.g., {0.6f, 0.2f, 0.15f, 1.0f}
    glm::vec4 mortarColor;  // e.g., {0.8f, 0.8f, 0.8f, 1.0f}
  };

  static constexpr auto kPushConstantRange = vk::PushConstantRange{
      vk::ShaderStageFlagBits::eCompute,
      0,
      sizeof(PushConstants),
  };

  using DescriptorSetLayouts = std::tuple<const dsl::BindlessLayout&>;

  explicit ProceduralTextureLayout(vk::Device device,
                                   DescriptorSetLayouts setLayouts)
      : PipelineLayout{
            device,
            createPipelineCreateInfo(setLayouts),
        } {}

 private:
  static auto createPipelineCreateInfo(DescriptorSetLayouts setLayouts)
      -> vk::PipelineLayoutCreateInfo {
    const auto set_layouts = std::array<vk::DescriptorSetLayout, 1>{
        *std::get<0>(setLayouts),
    };

    return vk::PipelineLayoutCreateInfo{{}, set_layouts, kPushConstantRange};
  }
};
};  // namespace vkit::vulkan::pl
