#pragma once

#include <filesystem>
#include <unordered_map>

#include "fastgltf/types.hpp"

namespace vkit::gltf {
class AssetExternalBuffers {
  std::unordered_map<std::size_t, std::vector<std::byte>>
      external_buffer_bytes_;
  std::vector<std::unique_ptr<std::byte[]>> meshopt_decompressed_bytes_;
  std::vector<std::span<const std::byte>> buffer_view_bytes_;

 public:
  AssetExternalBuffers(const fastgltf::Asset& asset,
                       const std::filesystem::path& directory);

  [[nodiscard]] std::span<const std::byte> operator()(
      const fastgltf::Asset& asset, std::size_t buffer_view_idx) const;
};
};  // namespace vkit::gltf
