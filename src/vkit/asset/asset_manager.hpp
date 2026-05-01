#pragma once

#include <filesystem>
#include <memory>

#include "vkit/graphics/device.hpp"
#include "vkit/graphics/util.hpp"
#include "vkit/material/storage.hpp"
#include "vkit/texture/loader.hpp"
#include "vkit/texture/storage.hpp"
#include "vkit/texture/texture.hpp"

namespace vkit::asset {

class AssetManager {
 public:
  AssetManager(const graphics::GfxDevice& gfxDevice,
               std::shared_ptr<texture::TextureStorage> textureStorage,
               std::shared_ptr<material::MaterialStorage> materialStorage)
      : gfxDevice_{gfxDevice},
        textureStorage_{std::move(textureStorage)},
        materialStorage_{std::move(materialStorage)} {}

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

    if (textureStorage_) {
      textureStorage_->add(logical_texture);
    }

    return logical_texture;
  }

  [[nodiscard]] auto getTextureStorage() const
      -> std::shared_ptr<texture::TextureStorage> {
    return textureStorage_;
  }
  [[nodiscard]] auto getMaterialStorage() const
      -> std::shared_ptr<material::MaterialStorage> {
    return materialStorage_;
  }

 private:
  const graphics::GfxDevice& gfxDevice_;
  std::shared_ptr<texture::TextureStorage> textureStorage_;
  std::shared_ptr<material::MaterialStorage> materialStorage_;
};

}  // namespace vkit::asset
