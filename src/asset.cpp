#include "asset.hpp"

#include <fstream>
#include <memory>
#include <span>
#include <stdexcept>
#include <utility>
#include <variant>

#include "fastgltf/types.hpp"
#include "meshoptimizer.h"

namespace {
auto load_file_as_binary(const std::filesystem::path& path)
    -> std::vector<std::byte> {
  auto file = std::ifstream{path, std::ios::binary | std::ios::ate};
  if (!file) {
    throw std::runtime_error{"failed to open file: " + path.string()};
  }

  const auto size = file.tellg();
  if (size < 0) {
    throw std::runtime_error{"failed to determine file size: " + path.string()};
  }

  std::vector<std::byte> buffer(static_cast<std::size_t>(size));

  file.seekg(0, std::ios::beg);
  if (!file.read(reinterpret_cast<char*>(buffer.data()), size)) {
    throw std::runtime_error{"failed to read file: " + path.string()};
  }

  return buffer;
}
};  // namespace

namespace vkit::gltf {
AssetExternalBuffers::AssetExternalBuffers(
    const fastgltf::Asset& asset, const std::filesystem::path& directory) {
  const auto get_buffers_bytes =
      [&](std::size_t buffer_idx) -> std::span<const std::byte> {
    return std::visit(
        fastgltf::visitor{
            [](const fastgltf::sources::Array& array)
                -> std::span<const std::byte> {
              return std::as_bytes(std::span{array.bytes});
            },
            [](const fastgltf::sources::ByteView& byte_view)
                -> std::span<const std::byte> { return byte_view.bytes; },
            [&](const fastgltf::sources::URI& uri)
                -> std::span<const std::byte> {
              auto it = external_buffer_bytes_.find(buffer_idx);
              if (it == external_buffer_bytes_.end()) {
                if (!uri.uri.isLocalPath()) {
                  throw std::runtime_error{
                      format_as(AssetProcessError::kUnsupportedSourceDataType),
                  };
                }
                it = external_buffer_bytes_.emplace_hint(
                    it, buffer_idx,
                    load_file_as_binary(directory / uri.uri.fspath()));
              }

              return it->second;
            },
            [](const auto&) -> std::span<const std::byte> {
              throw std::runtime_error{
                  format_as(AssetProcessError::kUnsupportedSourceDataType),
              };
            },
        },
        asset.buffers[buffer_idx].data);
  };

  buffer_view_bytes_.reserve(asset.bufferViews.size());
  for (const auto& buffer_view : asset.bufferViews) {
    if (const auto& mc = buffer_view.meshoptCompression) {
      const auto* compressed = reinterpret_cast<const unsigned char*>(
          get_buffers_bytes(mc->bufferIndex).data());

      const auto decompressed_buffer_size = mc->count * mc->byteStride;
      std::byte* const decompressed =
          meshopt_decompressed_bytes_
              .emplace_back(std::make_unique_for_overwrite<std::byte[]>(
                  decompressed_buffer_size))
              .get();

      auto rc = -1;
      switch (mc->mode) {
        case fastgltf::MeshoptCompressionMode::Attributes:
          rc = meshopt_decodeVertexBuffer(decompressed, mc->count,
                                          mc->byteStride, compressed,
                                          mc->byteLength);
          break;
        case fastgltf::MeshoptCompressionMode::Triangles:
          rc =
              meshopt_decodeIndexBuffer(decompressed, mc->count, mc->byteStride,
                                        compressed, mc->byteLength);
          break;
        case fastgltf::MeshoptCompressionMode::Indices:
          rc = meshopt_decodeIndexSequence(decompressed, mc->count,
                                           mc->byteStride, compressed,
                                           mc->byteLength);
          break;
      }

      if (rc != 0) {
        throw std::runtime_error{
            "failed to decompress EXT_meshopt_compression compressed buffer "
            "view"};
      }

      switch (mc->filter) {
        case fastgltf::MeshoptCompressionFilter::None:
          break;
        case fastgltf::MeshoptCompressionFilter::Octahedral:
          meshopt_decodeFilterOct(decompressed, mc->count, mc->byteStride);
          break;
        case fastgltf::MeshoptCompressionFilter::Quaternion:
          meshopt_decodeFilterQuat(decompressed, mc->count, mc->byteStride);
          break;
        case fastgltf::MeshoptCompressionFilter::Exponential:
          meshopt_decodeFilterExp(decompressed, mc->count, mc->byteStride);
          break;
      }

      buffer_view_bytes_.emplace_back(decompressed, decompressed_buffer_size);
    } else {
      const auto buffer_bytes = get_buffers_bytes(buffer_view.bufferIndex);
      buffer_view_bytes_.push_back(
          buffer_bytes.subspan(buffer_view.byteOffset, buffer_view.byteLength));
    }
  }
}

auto AssetExternalBuffers::operator()(std::size_t buffer_view_idx) const
    -> std::span<const std::byte> {
  return buffer_view_bytes_[buffer_view_idx];
}

auto format_as(AssetProcessError error) noexcept -> std::string {
  switch (error) {
    case AssetProcessError::kUnsupportedSourceDataType:
      return "the source data type is not supported";
  };

  std::unreachable();
};
};  // namespace vkit::gltf
