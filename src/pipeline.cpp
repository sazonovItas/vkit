#include "pipeline.hpp"

#include <array>

namespace lvk {
namespace {
constexpr auto kViewportStatesV =
    vk::PipelineViewportStateCreateInfo({}, 1, {}, 1);

constexpr auto kDynamicStatesV = std::array{
    vk::DynamicState::eViewport,         vk::DynamicState::eScissor,
    vk::DynamicState::eLineWidth,        vk::DynamicState::ePrimitiveTopology,
    vk::DynamicState::ePolygonModeEXT,   vk::DynamicState::eDepthTestEnable,
    vk::DynamicState::eDepthWriteEnable,
};

constexpr auto toVkbool(bool const value) {
  return value ? vk::True : vk::False;
}

[[nodiscard]] constexpr auto createShaderStages(
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

[[nodiscard]] constexpr auto createDepthStencilState(
    std::uint8_t flags, const vk::CompareOp depthCompare) {
  const auto depth_test =
      (flags & PipelineFlag::kDepthTest) == PipelineFlag::kDepthTest;
  auto ret = vk::PipelineDepthStencilStateCreateInfo{};
  ret.setDepthTestEnable(toVkbool(depth_test)).setDepthCompareOp(depthCompare);

  return ret;
}

[[nodiscard]] constexpr auto createColorBlendFactor(const std::uint8_t flags) {
  auto ret = vk::PipelineColorBlendAttachmentState{};
  auto const alpha_blend = (flags & lvk::PipelineFlag::kAlphaBlend) ==
                           lvk::PipelineFlag::kAlphaBlend;
  using CCF = vk::ColorComponentFlagBits;
  ret.setColorWriteMask(CCF::eR | CCF::eG | CCF::eB | CCF::eA)
      .setBlendEnable(toVkbool(alpha_blend))
      .setSrcColorBlendFactor(vk::BlendFactor::eSrcAlpha)
      .setDstColorBlendFactor(vk::BlendFactor::eOneMinusConstantAlpha)
      .setColorBlendOp(vk::BlendOp::eAdd)
      .setSrcAlphaBlendFactor(vk::BlendFactor::eOne)
      .setDstAlphaBlendFactor(vk::BlendFactor::eZero)
      .setAlphaBlendOp(vk::BlendOp::eAdd);
  return ret;
}
};  // namespace
//
auto PipelineBuilder::build(const vk::PipelineLayout layout,
                            const PipelineState& state) const
    -> vk::UniquePipeline {
  auto const shader_stage_ci =
      createShaderStages(state.vertexShader, state.fragmentShader);

  auto multisample_state_ci = vk::PipelineMultisampleStateCreateInfo{};
  multisample_state_ci.setRasterizationSamples(ci_.samples)
      .setSampleShadingEnable(vk::False);

  auto const input_assembly_ci =
      vk::PipelineInputAssemblyStateCreateInfo{{}, state.topology};

  auto rasterization_state_ci = vk::PipelineRasterizationStateCreateInfo{};
  rasterization_state_ci.setPolygonMode(state.polygonMode)
      .setCullMode(state.cullMode);

  auto const depth_stencil_state_ci =
      createDepthStencilState(state.flags, state.depthCompare);

  auto const color_blend_attachment = createColorBlendFactor(state.flags);
  auto color_blend_state_ci = vk::PipelineColorBlendStateCreateInfo{};
  color_blend_state_ci.setAttachments(color_blend_attachment);

  auto dynamic_state_ci = vk::PipelineDynamicStateCreateInfo{};
  dynamic_state_ci.setDynamicStates(kDynamicStatesV);

  auto rendering_ci = vk::PipelineRenderingCreateInfo{};
  if (ci_.colorFormat != vk::Format::eUndefined) {
    rendering_ci.setColorAttachmentFormats(ci_.colorFormat);
  }

  rendering_ci.setDepthAttachmentFormat(ci_.depthFormat);

  auto pipeline_ci = vk::GraphicsPipelineCreateInfo{};
  pipeline_ci.setLayout(layout)
      .setStages(shader_stage_ci)
      .setPViewportState(&kViewportStatesV)
      .setPMultisampleState(&multisample_state_ci)
      .setPInputAssemblyState(&input_assembly_ci)
      .setPRasterizationState(&rasterization_state_ci)
      .setPDepthStencilState(&depth_stencil_state_ci)
      .setPColorBlendState(&color_blend_state_ci)
      .setPDynamicState(&dynamic_state_ci)
      .setPNext(&rendering_ci);

  auto ret = vk::Pipeline{};
  if (ci_.device.createGraphicsPipelines({}, 1, &pipeline_ci, {}, &ret) !=
      vk::Result::eSuccess) {
    throw std::runtime_error{"failed to create graphics pipeline"};
    return {};
  }

  return vk::UniquePipeline{ret, ci_.device};
}
};  // namespace lvk
