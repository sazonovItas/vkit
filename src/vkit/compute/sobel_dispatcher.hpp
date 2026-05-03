#pragma once

#include <memory>
#include <vector>

#include "vkit/compute/async_compute.hpp"
#include "vkit/compute/pipeline_layout/operators.hpp"
#include "vkit/core/events/operators.hpp"
#include "vkit/graphics/device.hpp"
#include "vkit/message_bus/message_bus.hpp"

namespace vkit::compute {

struct InFlightSobelJob {
  uint64_t requestId;
  std::shared_ptr<texture::Texture> outputF32;
  std::shared_ptr<texture::Texture> outputUnorm;
  vk::UniqueDescriptorPool descriptorPool;
  vk::UniqueImageView viewF32;
  vk::UniqueImageView viewUnorm;
  vk::UniqueFence fence;
  ComputeResult computeResult;
  std::shared_ptr<ComputeTask> task;
};

class SobelDispatcher {
 public:
  SobelDispatcher(const graphics::GfxDevice& device,
                  core::events::SobelJobBus& jobBus,
                  core::events::SobelResultBus& resultBus,
                  std::shared_ptr<AsyncCompute> asyncCompute);

  void update();

 private:
  void onRequest(core::events::SobelJobRequest& req);

  const graphics::GfxDevice& device_;
  core::events::SobelResultBus& resultBus_;
  std::shared_ptr<AsyncCompute> asyncCompute_;

  std::unique_ptr<dsl::ImageOperatorSetLayout> setLayout_;
  std::unique_ptr<pl::SobelPipelineLayout> pipelineLayout_;
  vk::UniquePipeline pipeline_;

  vk::UniqueSampler sampler_;
  message_bus::Subscription<core::events::SobelJobRequest> sub_;
  std::vector<InFlightSobelJob> inFlightJobs_;
};

};  // namespace vkit::compute
