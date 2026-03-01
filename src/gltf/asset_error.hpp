#pragma once

namespace vkit::gltf {
enum class AssetProcessError : std::uint8_t {
  kUnsupportedSourceDataType,
};

auto format_as(AssetProcessError error) noexcept -> std::string;

};  // namespace vkit::gltf
