#pragma once

#include "asset_external_buffers.hpp"
#include "fastgltf/core.hpp"
#include "fastgltf/types.hpp"

namespace vkit::gltf {
class Asset {
  fastgltf::GltfDataBuffer data_buffer_;

 public:
  std::filesystem::path directory;
  fastgltf::Asset asset;

  AssetExternalBuffers external_buffers{asset, directory};

  std::size_t scene_idx;

  explicit Asset(const std::filesystem::path& path);

  [[nodiscard]] fastgltf::Scene& get_scene() noexcept {
    return asset.scenes[scene_idx];
  }

  [[nodiscard]] auto get_scene() const noexcept -> const fastgltf::Scene& {
    return asset.scenes[scene_idx];
  }

  void set_scene(std::size_t scene_idx) { this->scene_idx = scene_idx; }
};
}  // namespace vkit::gltf
