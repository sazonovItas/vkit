#include "primitive_attribute_buffers.hpp"

#include <unordered_set>

#include "fastgltf/types.hpp"
#include "vulkan/vma/allocator.hpp"

namespace vkit::gltf {
template <typename BufferDataAdapter>
PrimitiveAttributeBuffers::PrimitiveAttributeBuffers(
    const fastgltf::Asset& asset, const fastgltf::Primitive& primitive,
    const vulkan::vma::Allocator& allocator,
    const Config<BufferDataAdapter>& config)
    : asset_{asset}, primitive_{primitive} {}

template <typename BufferDataAdapter>
PrimitiveAttributeBuffers::AttributeInfoCache::AttributeInfoCache(
    const fastgltf::Asset& asset, const vulkan::vma::Allocator& allocator,
    const Config<BufferDataAdapter>& config) {
  std::unordered_set<std::size_t> accessors;
  std::unordered_set<std::size_t> buffer_views;

  for (const auto& mesh : asset.meshes) {
    for (const auto& primitive : mesh.primitives) {
      if (primitive.findAttribute("POSITION") == primitive.attributes.end()) {
        continue;
      }

      for (const auto& [attribute_name, accessor_idx] : primitive.attributes) {
        const auto& accessor = asset.accessors[accessor_idx];
        if (!accessor.bufferViewIndex) continue;

        accessors.emplace(accessor_idx);
        buffer_views.emplace(accessor.bufferViewIndex);
      }
    }
  }

  for (const auto& buffer_view_idx : buffer_views) {
    const std::span<const std::byte> buffer_view_data =
        config.adapter(asset, buffer_view_idx);
  }
}

auto PrimitiveAttributeBuffers::AttributeInfoCache::
    create_attribute_info_from_buffer(const fastgltf::Asset& asset,
                                      const fastgltf::Accessor& accessor) const
    -> PrimitiveAttributeBuffers::AttributeInfo {
  const auto buffer_view_idx = *accessor.bufferViewIndex;
  const auto& buffer_view = asset.bufferViews[buffer_view_idx];
  const auto element_byte_size =
      fastgltf::getElementByteSize(accessor.type, accessor.componentType);
  const auto stride = buffer_view.byteStride.value_or(element_byte_size);
  return {
      .buffer = buffer_view_buffers.at(buffer_view_idx),
      .offset = accessor.byteOffset,
      .size = (stride * (accessor.count - 1)) + element_byte_size,
      .stride = stride,
      .component_type = accessor.componentType,
  };
}
};  // namespace vkit::gltf
