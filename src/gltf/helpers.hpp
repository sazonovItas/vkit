#pragma once

#include <concepts>

#include "fastgltf/core.hpp"
#include "fastgltf/tools.hpp"
#include "fastgltf/types.hpp"

namespace vkit::gltf {
template <typename BufferDataAdapter = fastgltf::DefaultBufferDataAdapter>
[[nodiscard]] auto get_byte_region(const fastgltf::Asset& asset,
                                   const fastgltf::Accessor& accessor,
                                   const BufferDataAdapter& adapter = {})
    -> std::span<const std::byte> {
  const auto& buffer_view = asset.bufferViews[accessor.bufferViewIndex.value()];
  const auto element_byte_size =
      fastgltf::getElementByteSize(accessor.type, accessor.componentType);
  const auto byte_stride = buffer_view.byteStride.value_or(element_byte_size);
  return adapter(asset, *accessor.bufferViewIndex)
      .subspan(accessor.byteOffset,
               (byte_stride * (accessor.count - 1)) + element_byte_size);
}

template <std::move_constructible T>
[[nodiscard]] auto get_checked(fastgltf::Expected<T> expected) -> T {
  if (expected) {
    return std::move(expected.get());
  }

  throw expected.error();
}
};  // namespace vkit::gltf
