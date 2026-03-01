#include "asset_error.hpp"

namespace vkit::gltf {
auto format_as(AssetProcessError error) noexcept -> std::string {
  switch (error) {
    case AssetProcessError::kUnsupportedSourceDataType:
      return "the source data type is not supported";
  };

  std::unreachable();
};
};  // namespace vkit::gltf
