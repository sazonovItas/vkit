#pragma once

namespace vkit::util {

template <typename T>
[[nodiscard]] constexpr auto toByteSpan(std::vector<T> const& v) {
  return std::as_bytes(std::span{v});
}

};  // namespace vkit::util
