#pragma once

namespace vkit::workflow {

enum class PinType : std::size_t {
  kNone = 0,
  kColor = 1,
};

[[nodiscard]] inline auto pinKeyType(const PinType type) -> std::size_t {
  return static_cast<std::size_t>(type);
}

}  // namespace vkit::workflow
