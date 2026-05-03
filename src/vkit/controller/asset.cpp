#include "vkit/controller/asset.hpp"

namespace vkit::controller {

AssetController::AssetController(asset::AssetManager* assetManager)
    : assetManager_{assetManager} {}

auto AssetController::loadAssetFromFile() -> std::optional<std::uint32_t> {
  if (!assetManager_) return std::nullopt;

  auto asset = assetManager_->promptAndLoadGltf();

  if (asset) {
    auto id = asset->getStorageId();
    if (id.has_value()) {
      setCurrentAsset(id.value());
      return id;
    }
  }

  return std::nullopt;
}

void AssetController::setCurrentAsset(std::uint32_t id) {
  if (!assetManager_) return;

  auto storage = assetManager_->getGltfStorage();
  if (storage && storage->get(id)) {
    currentAssetId_ = id;
  }
}

void AssetController::clearCurrentAsset() { currentAssetId_ = std::nullopt; }

auto AssetController::getCurrentAssetId() const
    -> std::optional<std::uint32_t> {
  return currentAssetId_;
}

auto AssetController::getCurrentAsset() const -> std::shared_ptr<asset::Asset> {
  if (!assetManager_ || !currentAssetId_.has_value()) return nullptr;

  auto storage = assetManager_->getGltfStorage();
  if (storage) {
    return storage->get(currentAssetId_.value());
  }

  return nullptr;
}

auto AssetController::getLoadedAssets() const
    -> std::vector<std::shared_ptr<asset::Asset>> {
  if (!assetManager_) return {};

  auto storage = assetManager_->getGltfStorage();
  if (storage) {
    return storage->getItems();
  }

  return {};
}

void AssetController::removeAsset(std::uint32_t id) {
  if (!assetManager_) return;

  assetManager_->removeGltf(id);
}

};  // namespace vkit::controller
