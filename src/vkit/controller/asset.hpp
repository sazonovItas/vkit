#pragma once

#include <memory>
#include <optional>
#include <vector>

#include "vkit/asset/asset.hpp"
#include "vkit/asset/asset_manager.hpp"

namespace vkit::controller {

class AssetController {
 public:
  explicit AssetController(asset::AssetManager* assetManager);

  auto loadAssetFromFile() -> std::optional<std::uint32_t>;

  void setCurrentAsset(std::uint32_t id);
  void clearCurrentAsset();

  [[nodiscard]] auto getCurrentAssetId() const -> std::optional<std::uint32_t>;
  [[nodiscard]] auto getCurrentAsset() const -> std::shared_ptr<asset::Asset>;

  [[nodiscard]] auto getLoadedAssets() const
      -> std::vector<std::shared_ptr<asset::Asset>>;

  void removeAsset(std::uint32_t id);

 private:
  asset::AssetManager* assetManager_{nullptr};
  std::optional<std::uint32_t> currentAssetId_{std::nullopt};
};

};  // namespace vkit::controller
