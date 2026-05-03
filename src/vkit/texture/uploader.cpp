#include "vkit/texture/uploader.hpp"

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
        auto color_opts = opts;
        auto loaded_color =
            loadFromFile(device_.get(), device_.allocator, path, color_opts);

        auto f32_opts = opts;
        f32_opts.useMipmaps = false;
        f32_opts.isHdr = true;
        auto loaded_f32 =
            loadFromFile(device_.get(), device_.allocator, path, f32_opts);

        return std::make_pair(std::move(loaded_color), std::move(loaded_f32));
      });

  inFlightLoads_.push_back(InFlightDiskLoad{
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
        auto [loadedColor, loadedF32] = it->future.get();

        auto color_tex = std::make_shared<Texture>(
            it->path.filename().string() + "_color", loadedColor.texture);
        auto f32_tex = std::make_shared<Texture>(
            it->path.filename().string() + "_f32", loadedF32.texture);

        auto alloc_info =
            vk::CommandBufferAllocateInfo{}
                .setCommandPool(device_.getGraphicsPresentCommandPool())
                .setLevel(vk::CommandBufferLevel::ePrimary)
                .setCommandBufferCount(1);

        auto cb = std::move(
            device_.get().allocateCommandBuffersUnique(alloc_info)[0]);

        cb->begin(vk::CommandBufferBeginInfo{}.setFlags(
            vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

        loadedColor.texture->recordUpload(*cb, loadedColor.stagingBuffer);
        if (it->options.useMipmaps) {
          loadedColor.texture->recordMipmapGeneration(*cb);
        }

        loadedF32.texture->recordUpload(*cb, loadedF32.stagingBuffer);

        cb->end();

        auto fence = device_.get().createFenceUnique({});
        auto submit_info = vk::SubmitInfo{}.setCommandBuffers(*cb);

        device_.queues.graphicsPresent.submit(submit_info, *fence);

        inFlightUploads_.push_back(InFlightGpuUpload{
            .requestId = it->requestId,
            .logicalColor = std::move(color_tex),
            .logicalF32 = std::move(f32_tex),
            .fence = std::move(fence),
            .commandBuffer = std::move(cb),
            .stagingColor = std::move(loadedColor.stagingBuffer),
            .stagingF32 = std::move(loadedF32.stagingBuffer),
        });

      } catch (const std::exception& e) {
        readyBus_.sendMessage({
            .requestId = it->requestId,
            .color = nullptr,
            .image = nullptr,
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
      readyBus_.sendMessage({
          .requestId = it->requestId,
          .color = it->logicalColor,
          .image = it->logicalF32,
          .error = "",
      });

      it = inFlightUploads_.erase(it);
    } else {
      ++it;
    }
  }
}

};  // namespace vkit::texture
