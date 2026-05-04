#pragma once

#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "vkit/compute/async_compute.hpp"
#include "vkit/core/events/compute_output.hpp"
#include "vkit/graphics/device.hpp"
#include "vkit/message_bus/message_bus.hpp"
#include "vkit/texture/manager.hpp"

namespace vkit::compute {

struct InFlightComputeOutputJob {
  std::uint64_t requestId{0};
  std::shared_ptr<texture::Texture> outputF32;
  std::shared_ptr<texture::Texture> outputUnorm;
  vk::UniqueDescriptorPool descriptorPool;
  vk::UniqueImageView viewF32;
  vk::UniqueImageView viewUnormStorage;
  vk::UniqueFence fence;
  ComputeResult computeResult;
  std::shared_ptr<ComputeTask> task;
};

class ComputeOutputDispatcher {
 public:
  ComputeOutputDispatcher(const graphics::GfxDevice& device,
                          core::events::ComputeOutputBus& jobBus,
                          core::events::ComputeOutputResultBus& resultBus,
                          std::shared_ptr<AsyncCompute> asyncCompute);

  void registerPipeline(const std::string& name, vk::Device device,
                        const std::filesystem::path& spv,
                        core::events::ComputeBindingLayout layout,
                        uint32_t pushConstantsSize);

  [[nodiscard]] auto getPipeline(const std::string& name) const
      -> core::events::ComputeHandles;

  void update();

 private:
  void onRequest(core::events::ComputeOutputJob& job);

  [[nodiscard]] auto getDsl(core::events::ComputeBindingLayout l) const
      -> vk::DescriptorSetLayout;

  struct PipelineEntry {
    vk::UniquePipeline pipeline;
    vk::UniquePipelineLayout pipelineLayout;
    core::events::ComputeBindingLayout bindingLayout;
  };

  const graphics::GfxDevice& device_;
  core::events::ComputeOutputResultBus& resultBus_;
  std::shared_ptr<AsyncCompute> asyncCompute_;

  vk::UniqueDescriptorSetLayout generatorDsl_;
  vk::UniqueDescriptorSetLayout singleInputDsl_;
  vk::UniqueDescriptorSetLayout dualInputDsl_;

  vk::UniqueSampler samplerRepeat_;
  vk::UniqueSampler samplerClamp_;

  std::unordered_map<std::string, PipelineEntry> pipelines_;

  message_bus::Subscription<core::events::ComputeOutputJob> sub_;
  std::vector<InFlightComputeOutputJob> inFlightJobs_;
};

}  // namespace vkit::compute
