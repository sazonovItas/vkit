#pragma once

#include <filesystem>
#include <vulkan/vulkan.hpp>

#include "vkit/compute/pipeline_layout/noise.hpp"
#include "vkit/core/events/noise.hpp"

namespace vkit::compute {

class NoisePipeline {
 public:
  NoisePipeline(vk::Device device, const pl::NoisePipelineLayout& layout,
                const std::filesystem::path& shader_spv);

  [[nodiscard]] auto get() const -> vk::Pipeline { return *pipeline_; }

  void recordDispatch(vk::CommandBuffer cb, vk::PipelineLayout layout,
                      vk::DescriptorSet ds,
                      const core::events::NoisePushConstants& pc) const;

 private:
  vk::UniquePipeline pipeline_;
};

};  // namespace vkit::compute
