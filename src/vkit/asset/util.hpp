#pragma once

#include <filesystem>

namespace vkit::asset {

[[nodiscard]] auto locateAssetsDir() -> const std::filesystem::path&;

[[nodiscard]] auto assetPath(std::string_view uri) -> std::filesystem::path;

};  // namespace vkit::asset
