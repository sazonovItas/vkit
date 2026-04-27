#pragma once

#include "vkit/graphics/pipeline/compute.hpp"

namespace vkit::graphics::pipeline {

class ComputePipelineFactory {
 public:
  static auto create(vk::Device device, vk::PipelineLayout layout,
                     vk::PipelineShaderStageCreateInfo stage)
      -> ComputePipeline {
    return ComputePipelineBuilder{layout}.setShaderStage(stage).build(device);
  }
};

}  // namespace vkit::graphics::pipeline
