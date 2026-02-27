#pragma once

#include <filesystem>
#include <unordered_map>

#include "fastgltf/types.hpp"

namespace vkit::gltf {
enum class AssetProcessError : std::uint8_t {
  kUnsupportedSourceDataType,
};

auto format_as(AssetProcessError error) noexcept -> std::string;

class AssetExternalBuffers {
  std::unordered_map<std::size_t, std::vector<std::byte>>
      external_buffer_bytes_;
  std::vector<std::unique_ptr<std::byte[]>> meshopt_decompressed_bytes_;
  std::vector<std::span<const std::byte>> buffer_view_bytes_;

 public:
  AssetExternalBuffers(const fastgltf::Asset& asset,
                       const std::filesystem::path& directory);

  [[nodiscard]] std::span<const std::byte> operator()(
      std::size_t buffer_view_idx) const;
};

class Asset {
 public:
  std::filesystem::path direcotry;
  fastgltf::Asset asset;
};
};  // namespace vkit::gltf
