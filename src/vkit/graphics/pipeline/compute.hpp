#pragma once

#include "vkit/graphics/util.hpp"

namespace vkit::graphics::pipeline {

class ComputePipeline : public vk::UniquePipeline {
 public:
  ComputePipeline(vk::Device device,
                  const vk::ComputePipelineCreateInfo& createInfo,
                  const vk::PipelineCache& cache = {})
      : vk::UniquePipeline{createUniquePipeline(device, createInfo, cache)} {}

 private:
  static auto createUniquePipeline(
      vk::Device device, const vk::ComputePipelineCreateInfo& createInfo,
      const vk::PipelineCache& cache) -> vk::UniquePipeline {
    auto result = device.createComputePipelineUnique(cache, createInfo);
    util::requireSuccess(result.result,
                         "[PIPELINE] Failed to create compute pipeline");
    return std::move(result.value);
  }
};

class ComputePipelineBuilder {
 public:
  explicit ComputePipelineBuilder(vk::PipelineLayout layout) {
    pipelineInfo_.setLayout(layout);
  }

  auto setShaderStage(vk::PipelineShaderStageCreateInfo stage)
      -> ComputePipelineBuilder& {
    shaderStage_ = stage;
    return *this;
  }

  [[nodiscard]] auto build(vk::Device device, vk::PipelineCache cache = {})
      -> ComputePipeline {
    pipelineInfo_.setStage(shaderStage_);

    return ComputePipeline{device, pipelineInfo_, cache};
  }

 private:
  vk::ComputePipelineCreateInfo pipelineInfo_;
  vk::PipelineShaderStageCreateInfo shaderStage_;
};

}  // namespace vkit::graphics::pipeline
