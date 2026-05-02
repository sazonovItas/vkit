#pragma once

#include <memory>
#include <vector>

#include "vkit/compute/async_compute.hpp"
#include "vkit/core/events/compute.hpp"
#include "vkit/graphics/device.hpp"
#include "vkit/texture/manager.hpp"

namespace vkit::texture {

struct InFlightComputeJob {
  std::uint64_t requestId{0};
  std::shared_ptr<texture::Texture> outputTexture;
  vk::UniqueFence fence;
  compute::ComputeResult computeResult;
};

class ComputeDispatcher {
 public:
  ComputeDispatcher(const graphics::GfxDevice& device,
                    std::shared_ptr<TextureManager> storage,
                    core::events::ComputeJobBus& jobBus,
                    core::events::ComputeResultBus& resultBus,
                    std::shared_ptr<compute::AsyncCompute> asyncCompute);

  void update();

 private:
  void onRequest(core::events::ComputeJobRequest& req);

  const graphics::GfxDevice& device_;
  std::shared_ptr<TextureManager> storage_;
  core::events::ComputeResultBus& resultBus_;
  std::shared_ptr<compute::AsyncCompute> asyncCompute_;

  message_bus::Subscription<core::events::ComputeJobRequest> sub_;
  std::vector<InFlightComputeJob> inFlightJobs_;
};

};  // namespace vkit::texture
