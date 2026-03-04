#pragma once

#include "fastgltf/core.hpp"
#include "fastgltf/types.hpp"
#include "gltf/asset_external_buffers.hpp"

namespace gltf {
class Asset {
  fastgltf::GltfDataBuffer dataBuffer_;

 public:
  std::filesystem::path directory;
  fastgltf::Asset asset;

  AssetExternalBuffers externalBuffers{asset, directory};

  std::size_t sceneIdx;

  explicit Asset(const std::filesystem::path& path);

  [[nodiscard]] fastgltf::Scene& getScene() noexcept {
    return asset.scenes[sceneIdx];
  }

  [[nodiscard]] auto getScene() const noexcept -> const fastgltf::Scene& {
    return asset.scenes[sceneIdx];
  }

  void set_scene(std::size_t sceneIdx) { this->sceneIdx = sceneIdx; }
};
}  // namespace gltf
