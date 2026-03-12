#pragma once

namespace vkit {
inline constexpr std::size_t kResourceBufferingV{3};

template <typename Type>
using Buffered = std::array<Type, kResourceBufferingV>;
}  // namespace vkit
