#pragma once

#include <memory>
#include <vector>

#include "vkit/compute/async_compute.hpp"
#include "vkit/compute/descriptor_set_layout/image_operator.hpp"
#include "vkit/compute/pipeline_layout/operators.hpp"
#include "vkit/core/events/operators.hpp"
#include "vkit/graphics/device.hpp"
#include "vkit/message_bus/message_bus.hpp"

namespace vkit::compute {

struct InFlightNormalMapJob {
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

class NormalMapDispatcher {
 public:
  NormalMapDispatcher(const graphics::GfxDevice& device,
                      core::events::NormalMapJobBus& jobBus,
                      core::events::NormalMapResultBus& resultBus,
                      std::shared_ptr<AsyncCompute> asyncCompute);

  void update();

 private:
  void onRequest(core::events::NormalMapJobRequest& req);

  const graphics::GfxDevice& device_;
  core::events::NormalMapResultBus& resultBus_;
  std::shared_ptr<AsyncCompute> asyncCompute_;

  std::unique_ptr<dsl::ImageOperatorSetLayout> setLayout_;
  std::unique_ptr<pl::NormalMapPipelineLayout> pipelineLayout_;
  vk::UniquePipeline pipeline_;

  vk::UniqueSampler sampler_;
  message_bus::Subscription<core::events::NormalMapJobRequest> sub_;
  std::vector<InFlightNormalMapJob> inFlightJobs_;
};

};  // namespace vkit::compute
