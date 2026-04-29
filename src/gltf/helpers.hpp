// #pragma once
//
// #include <concepts>
//
// #include "fastgltf/core.hpp"
//
// namespace gltf {
// template <std::move_constructible T>
// [[nodiscard]] auto get_checked(fastgltf::Expected<T> expected) -> T {
//   if (expected) {
//     return std::move(expected.get());
//   }
//
//   throw expected.error();
// }
// };  // namespace gltf
