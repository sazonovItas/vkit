#pragma once

#include <filesystem>

#include "vkit/core/events/noise.hpp"

namespace vkit::compute {

class NoisePipeline {
 public:
  NoisePipeline(vk::Device device, const std::filesystem::path& shader_spv);

  [[nodiscard]] auto descriptorSetLayout() const -> vk::DescriptorSetLayout {
    return *dsl_;
  }
  [[nodiscard]] auto pipelineLayout() const -> vk::PipelineLayout {
    return *layout_;
  }
  [[nodiscard]] auto pipeline() const -> vk::Pipeline { return *pipeline_; }

  void recordDispatch(vk::CommandBuffer cb, vk::DescriptorSet ds,
                      const core::events::NoisePushConstants& pc) const;

 private:
  vk::UniqueDescriptorSetLayout dsl_;
  vk::UniquePipelineLayout layout_;
  vk::UniquePipeline pipeline_;
};

};  // namespace vkit::compute
