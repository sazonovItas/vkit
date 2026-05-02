#pragma once

#include <filesystem>
#include <memory>
#include <vector>

#include "vkit/compute/async_compute.hpp"
#include "vkit/compute/pipeline/noise.hpp"
#include "vkit/core/events/noise.hpp"
#include "vkit/graphics/device.hpp"
#include "vkit/message_bus/message_bus.hpp"
#include "vkit/texture/manager.hpp"

namespace vkit::compute {

struct InFlightNoiseJob {
  std::uint64_t requestId{0};
  std::shared_ptr<texture::Texture> outputF32;
  std::shared_ptr<texture::Texture> outputUnorm;
  vk::UniqueDescriptorPool descriptorPool;
  vk::UniqueImageView viewF32;
  vk::UniqueImageView viewUnorm;
  vk::UniqueFence fence;
  ComputeResult computeResult;
};

class NoiseDispatcher {
 public:
  NoiseDispatcher(const graphics::GfxDevice& device,
                  std::shared_ptr<texture::TextureManager> storage,
                  core::events::NoiseJobBus& jobBus,
                  core::events::NoiseResultBus& resultBus,
                  std::shared_ptr<AsyncCompute> asyncCompute);

  void update();

 private:
  void onRequest(core::events::NoiseJobRequest& req);

  const graphics::GfxDevice& device_;
  std::shared_ptr<texture::TextureManager> storage_;
  core::events::NoiseResultBus& resultBus_;
  std::shared_ptr<AsyncCompute> asyncCompute_;
  NoisePipeline pipeline_;
  vk::UniqueSampler sampler_;

  message_bus::Subscription<core::events::NoiseJobRequest> sub_;
  std::vector<InFlightNoiseJob> inFlightJobs_;
};

};  // namespace vkit::compute
