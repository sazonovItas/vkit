#pragma once

namespace vkit::workflow {

enum class PinType : std::size_t {
  kNone = 0,
  kColorTexture2D = 1,
  kFloatTexture2D = 2,
  kMaterial = 3,
};

[[nodiscard]] inline auto pinKeyType(const PinType type) -> std::size_t {
  return static_cast<std::size_t>(type);
}

}  // namespace vkit::workflow
