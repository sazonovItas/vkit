#pragma once

#include <algorithm>

#include "vkit/graphics/util.hpp"

namespace vkit::graphics::pipeline {

class GraphicsPipeline : public vk::UniquePipeline {
 public:
  GraphicsPipeline(vk::Device device,
                   const vk::GraphicsPipelineCreateInfo& createInfo,
                   const vk::PipelineCache& cache)
      : vk::UniquePipeline{createUniquePipeline(device, createInfo, cache)} {}

 private:
  static auto createUniquePipeline(
      vk::Device device, const vk::GraphicsPipelineCreateInfo& createInfo,
      const vk::PipelineCache& cache) -> vk::UniquePipeline {
    auto result = device.createGraphicsPipelineUnique(cache, createInfo);
    util::requireSuccess(result.result,
                         "[PIPELINE] Failed to create graphics pipeline");
    return std::move(result.value);
  }
};

namespace blend {

inline constexpr auto kOpaque =
    vk::PipelineColorBlendAttachmentState{}
        .setColorWriteMask(
            vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
            vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA)
        .setBlendEnable(vk::False);

inline constexpr auto kAlpha =
    vk::PipelineColorBlendAttachmentState{}
        .setColorWriteMask(
            vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
            vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA)
        .setBlendEnable(vk::True)
        .setSrcColorBlendFactor(vk::BlendFactor::eSrcAlpha)
        .setDstColorBlendFactor(vk::BlendFactor::eOneMinusSrcAlpha)
        .setColorBlendOp(vk::BlendOp::eAdd)
        .setSrcAlphaBlendFactor(vk::BlendFactor::eOne)
        .setDstAlphaBlendFactor(vk::BlendFactor::eZero)
        .setAlphaBlendOp(vk::BlendOp::eAdd);

inline constexpr auto kAdditive =
    vk::PipelineColorBlendAttachmentState{}
        .setColorWriteMask(
            vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
            vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA)
        .setBlendEnable(vk::True)
        .setSrcColorBlendFactor(vk::BlendFactor::eOne)
        .setDstColorBlendFactor(vk::BlendFactor::eOne)
        .setColorBlendOp(vk::BlendOp::eAdd)
        .setSrcAlphaBlendFactor(vk::BlendFactor::eOne)
        .setDstAlphaBlendFactor(vk::BlendFactor::eZero)
        .setAlphaBlendOp(vk::BlendOp::eAdd);

};  // namespace blend

class GraphicsPipelineBuilder {
 public:
  explicit GraphicsPipelineBuilder(vk::PipelineLayout layout) {
    pipelineInfo_.setLayout(layout);

    inputAssembly_ = vk::PipelineInputAssemblyStateCreateInfo{}
                         .setTopology(vk::PrimitiveTopology::eTriangleList)
                         .setPrimitiveRestartEnable(vk::False);

    viewportState_ = vk::PipelineViewportStateCreateInfo{}
                         .setViewportCount(1)
                         .setScissorCount(1);

    rasterizer_ = vk::PipelineRasterizationStateCreateInfo{}
                      .setDepthClampEnable(vk::False)
                      .setRasterizerDiscardEnable(vk::False)
                      .setPolygonMode(vk::PolygonMode::eFill)
                      .setLineWidth(1.0F)
                      .setCullMode(vk::CullModeFlagBits::eBack)
                      .setFrontFace(vk::FrontFace::eCounterClockwise);

    multisampling_ = vk::PipelineMultisampleStateCreateInfo{}
                         .setSampleShadingEnable(vk::False)
                         .setRasterizationSamples(vk::SampleCountFlagBits::e1);

    colorBlendAttachments_.push_back(blend::kOpaque);

    depthStencil_ = vk::PipelineDepthStencilStateCreateInfo{}
                        .setDepthTestEnable(vk::True)
                        .setDepthWriteEnable(vk::True)
                        .setDepthCompareOp(vk::CompareOp::eLessOrEqual)
                        .setDepthBoundsTestEnable(vk::False)
                        .setStencilTestEnable(vk::False);

    dynamicStates_ = {vk::DynamicState::eViewport, vk::DynamicState::eScissor};
  }

  auto addShaderStage(vk::PipelineShaderStageCreateInfo stage)
      -> GraphicsPipelineBuilder& {
    shaderStages_.push_back(stage);
    return *this;
  }

  auto setRenderingFormats(const std::vector<vk::Format>& colorFormats,
                           vk::Format depthFormat) -> GraphicsPipelineBuilder& {
    colorFormats_ = colorFormats;
    renderingInfo_ = vk::PipelineRenderingCreateInfo{}
                         .setColorAttachmentFormats(colorFormats_)
                         .setDepthAttachmentFormat(depthFormat);
    pipelineInfo_.setPNext(&renderingInfo_);
    return *this;
  }

  auto setTopology(vk::PrimitiveTopology topology) -> GraphicsPipelineBuilder& {
    inputAssembly_.setTopology(topology);
    return *this;
  }

  auto setPolygonMode(vk::PolygonMode mode) -> GraphicsPipelineBuilder& {
    rasterizer_.setPolygonMode(mode);
    return *this;
  }

  auto setCullMode(vk::CullModeFlags cullMode) -> GraphicsPipelineBuilder& {
    rasterizer_.setCullMode(cullMode);
    return *this;
  }

  auto setFrontFace(vk::FrontFace frontFace) -> GraphicsPipelineBuilder& {
    rasterizer_.setFrontFace(frontFace);
    return *this;
  }

  auto setDepthState(vk::Bool32 testEnable, vk::Bool32 writeEnable,
                     vk::CompareOp compareOp = vk::CompareOp::eLessOrEqual)
      -> GraphicsPipelineBuilder& {
    depthStencil_.setDepthTestEnable(testEnable);
    depthStencil_.setDepthWriteEnable(writeEnable);
    depthStencil_.setDepthCompareOp(compareOp);
    return *this;
  }

  auto setStencilTest(vk::Bool32 enable) -> GraphicsPipelineBuilder& {
    depthStencil_.setStencilTestEnable(enable);
    return *this;
  }

  auto setMultisampling(vk::SampleCountFlagBits samples,
                        vk::Bool32 sampleShadingEnable = vk::False,
                        float minSampleShading = 1.0F)
      -> GraphicsPipelineBuilder& {
    multisampling_.setRasterizationSamples(samples);
    multisampling_.setSampleShadingEnable(sampleShadingEnable);
    multisampling_.setMinSampleShading(minSampleShading);
    return *this;
  }

  auto setColorBlendAttachment(
      std::size_t index,
      const vk::PipelineColorBlendAttachmentState& attachment)
      -> GraphicsPipelineBuilder& {
    if (index >= colorBlendAttachments_.size()) {
      colorBlendAttachments_.resize(index + 1, blend::kOpaque);
    }

    colorBlendAttachments_[index] = attachment;
    return *this;
  }

  auto clearColorBlendAttachments() -> GraphicsPipelineBuilder& {
    colorBlendAttachments_.clear();
    return *this;
  }

  auto addDynamicState(vk::DynamicState state) -> GraphicsPipelineBuilder& {
    if (std::ranges::find(dynamicStates_, state) == dynamicStates_.end()) {
      dynamicStates_.push_back(state);
    }
    return *this;
  }

  auto setVertexInput(
      vk::ArrayProxy<const vk::VertexInputBindingDescription> bindings,
      vk::ArrayProxy<const vk::VertexInputAttributeDescription> attributes)
      -> GraphicsPipelineBuilder& {
    vertexBindings_ = {bindings.begin(), bindings.end()};
    vertexAttributes_ = {attributes.begin(), attributes.end()};

    return *this;
  }

  [[nodiscard]] auto build(vk::Device device, vk::PipelineCache cache = {})
      -> GraphicsPipeline {
    colorBlending_ = vk::PipelineColorBlendStateCreateInfo{}
                         .setLogicOpEnable(vk::False)
                         .setAttachments(colorBlendAttachments_);

    dynamicStateInfo_ =
        vk::PipelineDynamicStateCreateInfo{}.setDynamicStates(dynamicStates_);

    auto vertex_input_state =
        vk::PipelineVertexInputStateCreateInfo{}
            .setVertexBindingDescriptions(vertexBindings_)
            .setVertexAttributeDescriptions(vertexAttributes_);

    pipelineInfo_.setStages(shaderStages_);
    pipelineInfo_.setPVertexInputState(&vertex_input_state);
    pipelineInfo_.setPInputAssemblyState(&inputAssembly_);
    pipelineInfo_.setPViewportState(&viewportState_);
    pipelineInfo_.setPRasterizationState(&rasterizer_);
    pipelineInfo_.setPMultisampleState(&multisampling_);
    pipelineInfo_.setPDepthStencilState(&depthStencil_);
    pipelineInfo_.setPColorBlendState(&colorBlending_);
    pipelineInfo_.setPDynamicState(&dynamicStateInfo_);

    return GraphicsPipeline{device, pipelineInfo_, cache};
  }

 private:
  vk::GraphicsPipelineCreateInfo pipelineInfo_;

  std::vector<vk::PipelineShaderStageCreateInfo> shaderStages_;

  std::vector<vk::VertexInputBindingDescription> vertexBindings_;
  std::vector<vk::VertexInputAttributeDescription> vertexAttributes_;

  vk::PipelineInputAssemblyStateCreateInfo inputAssembly_;
  vk::PipelineViewportStateCreateInfo viewportState_;
  vk::PipelineRasterizationStateCreateInfo rasterizer_;
  vk::PipelineMultisampleStateCreateInfo multisampling_;

  std::vector<vk::PipelineColorBlendAttachmentState> colorBlendAttachments_;
  vk::PipelineColorBlendStateCreateInfo colorBlending_;

  vk::PipelineDepthStencilStateCreateInfo depthStencil_;

  std::vector<vk::DynamicState> dynamicStates_;
  vk::PipelineDynamicStateCreateInfo dynamicStateInfo_;

  std::vector<vk::Format> colorFormats_;
  vk::PipelineRenderingCreateInfo renderingInfo_;
};

};  // namespace vkit::graphics::pipeline
