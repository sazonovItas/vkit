#pragma once

#include <memory>
#include <vector>

#include "vkit/compute/async_compute.hpp"
#include "vkit/compute/descriptor_set_layout/mix.hpp"
#include "vkit/compute/pipeline_layout/operators.hpp"
#include "vkit/core/events/operators.hpp"
#include "vkit/graphics/device.hpp"
#include "vkit/message_bus/message_bus.hpp"

namespace vkit::compute {

struct InFlightMixJob {
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

class MixDispatcher {
 public:
  MixDispatcher(const graphics::GfxDevice& device,
                core::events::MixJobBus& jobBus,
                core::events::MixResultBus& resultBus,
                std::shared_ptr<AsyncCompute> asyncCompute);

  void update();

 private:
  void onRequest(core::events::MixJobRequest& req);

  const graphics::GfxDevice& device_;
  core::events::MixResultBus& resultBus_;
  std::shared_ptr<AsyncCompute> asyncCompute_;

  std::unique_ptr<dsl::MixOperatorSetLayout> setLayout_;
  std::unique_ptr<pl::MixPipelineLayout> pipelineLayout_;
  vk::UniquePipeline pipeline_;

  vk::UniqueSampler sampler_;
  message_bus::Subscription<core::events::MixJobRequest> sub_;
  std::vector<InFlightMixJob> inFlightJobs_;
};

};  // namespace vkit::compute
