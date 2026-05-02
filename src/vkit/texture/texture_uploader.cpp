#include "vkit/texture/texture_uploader.hpp"

#include "vkit/texture/texture.hpp"

namespace vkit::texture {

TextureUploader::TextureUploader(const graphics::GfxDevice& device,
                                 std::shared_ptr<TextureManager> storage,
                                 core::events::TextureLoadBus& loadBus,
                                 core::events::TextureReadyBus& readyBus)
    : device_{device},
      storage_{std::move(storage)},
      readyBus_{readyBus},
      sub_{loadBus.subscribe(
          [this](core::events::TextureLoadRequest& req) { onRequest(req); })} {}

void TextureUploader::onRequest(core::events::TextureLoadRequest& req) {
  auto future = std::async(
      std::launch::async, [this, path = req.path, opts = req.options]() {
        return loadFromFile(device_.get(), device_.allocator, path, opts);
      });

  inFlightLoads_.push_back({
      .requestId = req.requestId,
      .path = req.path,
      .options = req.options,
      .future = std::move(future),
  });
}

void TextureUploader::update() {
  using namespace std::chrono_literals;

  for (auto it = inFlightLoads_.begin(); it != inFlightLoads_.end();) {
    if (it->future.wait_for(0s) == std::future_status::ready) {
      try {
        auto loaded = it->future.get();
        auto logical = std::make_shared<Texture>(it->path.filename().string(),
                                                 loaded.texture);

        auto alloc_info =
            vk::CommandBufferAllocateInfo{}
                .setCommandPool(device_.getGraphicsPresentCommandPool())
                .setLevel(vk::CommandBufferLevel::ePrimary)
                .setCommandBufferCount(1);

        auto cb = std::move(
            device_.get().allocateCommandBuffersUnique(alloc_info)[0]);

        cb->begin(vk::CommandBufferBeginInfo{}.setFlags(
            vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

        loaded.texture->recordUpload(*cb, loaded.stagingBuffer);
        if (it->options.useMipmaps) {
          loaded.texture->recordMipmapGeneration(*cb);
        }
        cb->end();

        auto fence = device_.get().createFenceUnique({});
        auto submit_info = vk::SubmitInfo{}.setCommandBuffers(*cb);

        device_.queues.graphicsPresent.submit(submit_info, *fence);

        inFlightUploads_.push_back({
            .requestId = it->requestId,
            .logicalTexture = std::move(logical),
            .fence = std::move(fence),
            .commandBuffer = std::move(cb),
            .stagingBuffer = std::move(loaded.stagingBuffer),
        });

      } catch (const std::exception& e) {
        readyBus_.sendMessage({
            .requestId = it->requestId,
            .texture = nullptr,
            .error = e.what(),
        });
      }
      it = inFlightLoads_.erase(it);
    } else {
      ++it;
    }
  }

  for (auto it = inFlightUploads_.begin(); it != inFlightUploads_.end();) {
    if (device_.get().getFenceStatus(*it->fence) == vk::Result::eSuccess) {
      if (storage_) {
        storage_->add(it->logicalTexture);
      }

      readyBus_.sendMessage({
          .requestId = it->requestId,
          .texture = it->logicalTexture,
          .error = "",
      });

      it = inFlightUploads_.erase(it);
    } else {
      ++it;
    }
  }
}

};  // namespace vkit::texture
