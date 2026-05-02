#include "vkit/texture/compute_dispatcher.hpp"

namespace vkit::texture {

ComputeDispatcher::ComputeDispatcher(
    const graphics::GfxDevice& device, std::shared_ptr<TextureManager> storage,
    core::events::ComputeJobBus& jobBus,
    core::events::ComputeResultBus& resultBus,
    std::shared_ptr<compute::AsyncCompute> asyncCompute)
    : device_{device},
      storage_{std::move(storage)},
      resultBus_{resultBus},
      asyncCompute_{std::move(asyncCompute)},
      sub_{jobBus.subscribe(
          [this](core::events::ComputeJobRequest& req) { onRequest(req); })} {}

void ComputeDispatcher::onRequest(core::events::ComputeJobRequest& req) {
  if (!req.task) {
    resultBus_.sendMessage({
        .requestId = req.requestId,
        .texture = nullptr,
        .error = "null compute task",
    });
    return;
  }

  auto fence = device_.get().createFenceUnique(vk::FenceCreateInfo{});

  auto compute_result = asyncCompute_->submit(*req.task, *fence);

  inFlightJobs_.push_back(InFlightComputeJob{
      .requestId = req.requestId,
      .outputTexture = req.outputTexture,
      .fence = std::move(fence),
      .computeResult = std::move(compute_result),
  });
}

void ComputeDispatcher::update() {
  for (auto it = inFlightJobs_.begin(); it != inFlightJobs_.end();) {
    if (device_.get().getFenceStatus(*it->fence) == vk::Result::eSuccess) {
      if (it->outputTexture && storage_) {
        storage_->add(it->outputTexture);
      }

      resultBus_.sendMessage(core::events::ComputeJobResult{
          .requestId = it->requestId,
          .texture = it->outputTexture,
      });

      it = inFlightJobs_.erase(it);
    } else {
      ++it;
    }
  }
}

};  // namespace vkit::texture
