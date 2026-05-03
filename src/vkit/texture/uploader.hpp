#pragma once

#include <future>
#include <memory>
#include <vector>

#include "vkit/core/events/texture.hpp"
#include "vkit/graphics/device.hpp"
#include "vkit/texture/loader.hpp"
#include "vkit/texture/manager.hpp"

namespace vkit::texture {

struct InFlightDiskLoad {
  std::uint64_t requestId;
  std::filesystem::path path;
  LoadOptions options;
  std::future<std::pair<LoadedTexture, LoadedTexture>> future;
};

struct InFlightGpuUpload {
  std::uint64_t requestId;
  std::shared_ptr<texture::Texture> logicalColor;
  std::shared_ptr<texture::Texture> logicalF32;
  vk::UniqueFence fence;
  vk::UniqueCommandBuffer commandBuffer;
  graphics::MappedBuffer stagingColor;
  graphics::MappedBuffer stagingF32;
};

class TextureUploader {
 public:
  TextureUploader(const graphics::GfxDevice& device,
                  std::shared_ptr<TextureManager> storage,
                  core::events::TextureLoadBus& loadBus,
                  core::events::TextureReadyBus& readyBus);

  void update();

 private:
  void onRequest(core::events::TextureLoadRequest& req);

  const graphics::GfxDevice& device_;
  std::shared_ptr<TextureManager> storage_;
  core::events::TextureReadyBus& readyBus_;

  message_bus::Subscription<core::events::TextureLoadRequest> sub_;

  std::vector<InFlightDiskLoad> inFlightLoads_;
  std::vector<InFlightGpuUpload> inFlightUploads_;
};

};  // namespace vkit::texture
