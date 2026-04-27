#include "vkit/graphics/pipeline/graphics.hpp"

namespace vkit::graphics::pipeline {

class GraphicsPipelineFactory {
 public:
  static auto createOpaque(
      vk::Device device, vk::PipelineLayout layout,
      vk::PipelineShaderStageCreateInfo vertStage,
      vk::PipelineShaderStageCreateInfo fragStage, vk::Format colorFormat,
      vk::Format depthFormat,
      vk::CullModeFlags cullMode = vk::CullModeFlagBits::eBack,
      vk::SampleCountFlagBits samples = vk::SampleCountFlagBits::e1)
      -> GraphicsPipeline {
    return GraphicsPipelineBuilder{layout}
        .addShaderStage(vertStage)
        .addShaderStage(fragStage)
        .setRenderingFormats({colorFormat}, depthFormat)
        .setColorBlendAttachment(0, blend::kOpaque)
        .setMultisampling(samples)
        .setCullMode(cullMode)
        .setDepthState(vk::True, vk::True)
        .build(device);
  }

  static auto createTransparent(
      vk::Device device, vk::PipelineLayout layout,
      vk::PipelineShaderStageCreateInfo vertStage,
      vk::PipelineShaderStageCreateInfo fragStage, vk::Format colorFormat,
      vk::Format depthFormat,
      vk::CullModeFlags cullMode = vk::CullModeFlagBits::eBack,
      vk::SampleCountFlagBits samples = vk::SampleCountFlagBits::e1)
      -> GraphicsPipeline {
    return GraphicsPipelineBuilder{layout}
        .addShaderStage(vertStage)
        .addShaderStage(fragStage)
        .setRenderingFormats({colorFormat}, depthFormat)
        .setColorBlendAttachment(0, blend::kAlpha)
        .setMultisampling(samples)
        .setCullMode(cullMode)
        .setDepthState(vk::True, vk::False)
        .build(device);
  }
};

};  // namespace vkit::graphics::pipeline
