#include "vkit/asset/util.hpp"

#include <print>

namespace vkit::asset {

auto locateAssetsDir() -> const std::filesystem::path& {
  static const std::filesystem::path kCachedPath =
      []() -> std::filesystem::path {
    static constexpr std::string_view kDirNameV{"assets"};

    for (auto path = std::filesystem::current_path();
         !path.empty() && path.has_parent_path(); path = path.parent_path()) {
      auto ret = path / kDirNameV;
      if (std::filesystem::is_directory(ret)) {
        return ret;
      }
    }

    std::println("[VKIT] [WARNING]: could not locate '{}' directory",
                 kDirNameV);

    return std::filesystem::current_path();
  }();

  return kCachedPath;
}

auto assetPath(std::string_view uri) -> std::filesystem::path {
  return locateAssetsDir() / uri;
}

};  // namespace vkit::asset
