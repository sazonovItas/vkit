#pragma once

#include "vkit/graphics/util.hpp"

namespace vkit::graphics {

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

};  // namespace vkit::graphics
