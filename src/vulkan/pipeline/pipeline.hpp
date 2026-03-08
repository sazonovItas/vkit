#pragma once

#include "vku/utils/utils.hpp"

namespace vkit::vulkan::pipeline {
class GraphicsPipeline : public vk::UniquePipeline {
 public:
  GraphicsPipeline(vk::Device device,
                   const vk::GraphicsPipelineCreateInfo& createInfo)
      : vk::UniquePipeline{createUniquePipeline(device, createInfo)} {}

 private:
  static auto createUniquePipeline(
      vk::Device device, const vk::GraphicsPipelineCreateInfo& createInfo)
      -> vk::UniquePipeline {
    auto result =
        device.createGraphicsPipelineUnique(vk::PipelineCache{}, createInfo);
    vku::requireSuccess(result.result, "failed to create graphics pipeline");
    return std::move(result.value);
  }
};
};  // namespace vkit::vulkan::pipeline
