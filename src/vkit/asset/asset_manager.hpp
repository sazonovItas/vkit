#pragma once

#include <array>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <vector>

#include "vkit/asset/gltf/importer.hpp"
#include "vkit/asset/gltf_storage.hpp"
#include "vkit/graphics/device.hpp"
#include "vkit/platform/file_dialog.hpp"

namespace vkit::asset {

class AssetManager {
 public:
  AssetManager(const graphics::GfxDevice& gfxDevice,
               std::shared_ptr<GltfStorage> gltfStorage,
               std::uint32_t maxFramesInFlight = 3)
      : gfxDevice_{gfxDevice},
        gltfStorage_{std::move(gltfStorage)},
        maxFramesInFlight_{maxFramesInFlight} {}

  AssetManager(const AssetManager&) = delete;
  auto operator=(const AssetManager&) -> AssetManager& = delete;

  [[nodiscard]] auto loadGltf(const std::filesystem::path& filepath)
      -> std::shared_ptr<Asset> {
    gltf::Importer importer(gfxDevice_);
    auto asset = importer.load(filepath);

    if (asset && gltfStorage_) {
      gltfStorage_->add(asset);
    }

    return asset;
  }

  [[nodiscard]] auto promptAndLoadGltf() -> std::shared_ptr<Asset> {
    std::array<platform::FileFilter, 1> filters = {
        platform::FileFilter{.name = "glTF Models", .spec = "gltf"}};

    auto selected_path = platform::openFileDialog(filters, "");

    if (selected_path.has_value()) {
      return loadGltf(selected_path.value());
    }

    return nullptr;
  }

  void removeGltf(std::uint32_t id) {
    if (!gltfStorage_) return;

    auto asset = gltfStorage_->get(id);
    if (asset) {
      gcQueue_.push_back(
          {.asset = std::move(asset),
           .framesRemaining = static_cast<int>(maxFramesInFlight_)});
      gltfStorage_->remove(id);
    }
  }

  void processGC() {
    for (auto it = gcQueue_.begin(); it != gcQueue_.end();) {
      if (it->framesRemaining > 0) {
        it->framesRemaining--;
        ++it;
      } else {
        it = gcQueue_.erase(it);
      }
    }
  }

  [[nodiscard]] auto getGltfStorage() const -> std::shared_ptr<GltfStorage> {
    return gltfStorage_;
  }

 private:
  const graphics::GfxDevice& gfxDevice_;
  std::shared_ptr<GltfStorage> gltfStorage_;

  struct GCTask {
    std::shared_ptr<Asset> asset;
    int framesRemaining;
  };

  std::vector<GCTask> gcQueue_;
  std::uint32_t maxFramesInFlight_{3};
};

};  // namespace vkit::asset
