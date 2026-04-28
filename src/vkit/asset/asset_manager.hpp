#pragma once

#include <filesystem>
#include <memory>

#include "vkit/graphics/device.hpp"
#include "vkit/graphics/util.hpp"
#include "vkit/texture/loader.hpp"
#include "vkit/texture/storage.hpp"
#include "vkit/texture/texture.hpp"

namespace vkit::asset {

class AssetManager {
 public:
  AssetManager(const graphics::GfxDevice& gfxDevice,
               texture::Storage& textureStorage)
      : gfxDevice_{gfxDevice}, textureStorage_{textureStorage} {}

  AssetManager(const AssetManager&) = delete;
  auto operator=(const AssetManager&) -> AssetManager& = delete;

  [[nodiscard]] auto loadTexture(const std::filesystem::path& filepath,
                                 const texture::LoadOptions& options = {})
      -> std::shared_ptr<texture::Texture> {
    auto loaded = texture::loadFromFile(gfxDevice_.get(), gfxDevice_.allocator,
                                        filepath, options);

    const auto submit_info = graphics::util::RecordAndSubmitInfo{
        .device = gfxDevice_.get(),
        .queue = gfxDevice_.queues.graphicsPresent,
        .commandPool = gfxDevice_.getGraphicsPresentCommandPool(),
    };

    graphics::util::recordAndSubmit(submit_info, [&](vk::CommandBuffer cb) {
      loaded.texture->recordUpload(cb, loaded.stagingBuffer);

      if (options.useMipmaps) {
        loaded.texture->recordMipmapGeneration(cb);
      }
    });

    const auto name = filepath.filename().string();
    auto logical_texture =
        std::make_shared<texture::Texture>(name, loaded.texture);

    textureStorage_.add(logical_texture);

    return logical_texture;
  }

 private:
  const graphics::GfxDevice& gfxDevice_;
  texture::Storage& textureStorage_;
};

}  // namespace vkit::asset
