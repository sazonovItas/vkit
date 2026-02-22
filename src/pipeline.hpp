#pragma once

#include <cstdint>

namespace lvk {
struct PipelineFlag {
  enum : std::uint8_t {
    kNone = 0,
    kAlphaBlend = 1 << 0,
    kDepthTest = 1 << 1,
  };
};

struct PipelineState {
  using Flag = PipelineFlag;

  [[nodiscard]] static constexpr auto default_flags() -> std::uint8_t {
    return Flag::kAlphaBlend | Flag::kDepthTest;
  }

  vk::ShaderModule vertex_shader;
  vk::ShaderModule fragment_shader;

  std::span<vk::VertexInputAttributeDescription const> vertex_attributes;
  std::span<vk::VertexInputBindingDescription const> vertex_bindings;

  vk::PrimitiveTopology topology{vk::PrimitiveTopology::eTriangleList};
  vk::PolygonMode polygon_mode{vk::PolygonMode::eFill};
  vk::CullModeFlags cull_mode{vk::CullModeFlagBits::eNone};
  vk::CompareOp depth_compare{vk::CompareOp::eLess};
  std::uint8_t flags{default_flags()};
};

struct PipelineBuilderCreateInfo {
  vk::Device device;
  vk::SampleCountFlagBits samples;
  vk::Format color_format;
  vk::Format depth_format;
};

class PipelineBuilder {
 public:
  using CreateInfo = PipelineBuilderCreateInfo;

  explicit PipelineBuilder(CreateInfo const& create_info)
      : m_info_{create_info} {}

  [[nodiscard]] auto build(vk::PipelineLayout layout,
                           PipelineState const& state) const
      -> vk::UniquePipeline;

 private:
  CreateInfo m_info_;
};

};  // namespace lvk
