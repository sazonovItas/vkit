#pragma once

namespace lvk {
// Number of virtual frames
inline constexpr std::size_t kResourceBufferingV{3};

// Alias for N-buffered resources
template <typename Type>
using Buffered = std::array<Type, kResourceBufferingV>;
}  // namespace lvk
