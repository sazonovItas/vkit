#include "pipeline.hpp"

#include <array>
#include <print>

#include "vulkan/vulkan.hpp"

namespace {
constexpr auto kViewportStatesV =
    vk::PipelineViewportStateCreateInfo({}, 1, {}, 1);

constexpr auto kDynamicStatesV = std::array{
    vk::DynamicState::eViewport,
    vk::DynamicState::eScissor,
    vk::DynamicState::eLineWidth,
    vk::DynamicState::ePolygonModeEXT,
};

constexpr auto to_vkbool(bool const value) {
  return value ? vk::True : vk::False;
}

[[nodiscard]] constexpr auto create_shader_stages(
    vk::ShaderModule const vertex, vk::ShaderModule const fragment) {
  auto ret = std::array<vk::PipelineShaderStageCreateInfo, 2>{};
  ret[0]
      .setStage(vk::ShaderStageFlagBits::eVertex)
      .setPName("main")
      .setModule(vertex);
  ret[1]
      .setStage(vk::ShaderStageFlagBits::eFragment)
      .setPName("main")
      .setModule(fragment);
  return ret;
}

[[nodiscard]] constexpr auto create_depth_stencil_state(
    std::uint8_t flags, vk::CompareOp const depth_compare) {
  auto ret = vk::PipelineDepthStencilStateCreateInfo{};
  auto const depth_test =
      (flags & lvk::PipelineFlag::kDepthTest) == lvk::PipelineFlag::kDepthTest;
  ret.setDepthTestEnable(to_vkbool(depth_test))
      .setDepthCompareOp(depth_compare);
  return ret;
}

[[nodiscard]] constexpr auto create_color_blend_attachment(
    std::uint8_t const flags) {
  auto ret = vk::PipelineColorBlendAttachmentState{};
  auto const alpha_blend = (flags & lvk::PipelineFlag::kAlphaBlend) ==
                           lvk::PipelineFlag::kAlphaBlend;
  using CCF = vk::ColorComponentFlagBits;
  ret.setColorWriteMask(CCF::eR | CCF::eG | CCF::eB | CCF::eA)
      .setBlendEnable(to_vkbool(alpha_blend))
      .setSrcColorBlendFactor(vk::BlendFactor::eSrcAlpha)
      .setDstColorBlendFactor(vk::BlendFactor::eOneMinusConstantAlpha)
      .setColorBlendOp(vk::BlendOp::eAdd)
      .setSrcAlphaBlendFactor(vk::BlendFactor::eOne)
      .setDstAlphaBlendFactor(vk::BlendFactor::eZero)
      .setAlphaBlendOp(vk::BlendOp::eAdd);
  return ret;
}
};  // namespace

namespace lvk {
auto PipelineBuilder::build(vk::PipelineLayout const layout,
                            PipelineState const& state) const
    -> vk::UniquePipeline {
  auto const shader_stage_ci =
      create_shader_stages(state.vertex_shader, state.fragment_shader);

  auto vertex_input_ci = vk::PipelineVertexInputStateCreateInfo{};
  vertex_input_ci.setVertexAttributeDescriptions(state.vertex_attributes)
      .setVertexBindingDescriptions(state.vertex_bindings);

  auto multisample_state_ci = vk::PipelineMultisampleStateCreateInfo{};
  multisample_state_ci.setRasterizationSamples(m_info_.samples)
      .setSampleShadingEnable(vk::False);

  auto const input_assembly_ci =
      vk::PipelineInputAssemblyStateCreateInfo{{}, state.topology};

  auto rasterization_state_ci = vk::PipelineRasterizationStateCreateInfo{};
  rasterization_state_ci.setPolygonMode(state.polygon_mode)
      .setCullMode(state.cull_mode);

  auto const depth_stencil_state_ci =
      create_depth_stencil_state(state.flags, state.depth_compare);

  auto const color_blend_attachment =
      create_color_blend_attachment(state.flags);
  auto color_blend_state_ci = vk::PipelineColorBlendStateCreateInfo{};
  color_blend_state_ci.setAttachments(color_blend_attachment);

  auto dynamic_state_ci = vk::PipelineDynamicStateCreateInfo{};
  dynamic_state_ci.setDynamicStates(kDynamicStatesV);

  auto rendering_ci = vk::PipelineRenderingCreateInfo{};
  if (m_info_.color_format != vk::Format::eUndefined) {
    rendering_ci.setColorAttachmentFormats(m_info_.color_format);
  }

  rendering_ci.setDepthAttachmentFormat(m_info_.depth_format);

  auto pipeline_ci = vk::GraphicsPipelineCreateInfo{};
  pipeline_ci.setLayout(layout)
      .setStages(shader_stage_ci)
      .setPVertexInputState(&vertex_input_ci)
      .setPViewportState(&kViewportStatesV)
      .setPMultisampleState(&multisample_state_ci)
      .setPInputAssemblyState(&input_assembly_ci)
      .setPRasterizationState(&rasterization_state_ci)
      .setPDepthStencilState(&depth_stencil_state_ci)
      .setPColorBlendState(&color_blend_state_ci)
      .setPDynamicState(&dynamic_state_ci)
      .setPNext(&rendering_ci);

  auto ret = vk::Pipeline{};
  if (m_info_.device.createGraphicsPipelines({}, 1, &pipeline_ci, {}, &ret) !=
      vk::Result::eSuccess) {
    std::println(stderr, "[lvk] failed to create Graphics Pipeline");
    return {};
  }

  return vk::UniquePipeline{ret, m_info_.device};
}
};  // namespace lvk
