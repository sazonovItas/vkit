#pragma once

#include <filesystem>
#include <memory>
#include <span>

#include "vkit/graphics/mapped_buffer.hpp"
#include "vkit/graphics/texture.hpp"

namespace vkit::texture {

struct LoadOptions {
  bool useMipmaps{true};
  bool isSrgb{true};
  bool isHdr{false};
  graphics::TextureType type{graphics::TextureType::k2D};
};

struct LoadedTexture {
  std::shared_ptr<graphics::Texture> texture;
  graphics::MappedBuffer stagingBuffer;
};

[[nodiscard]] auto loadFromFile(vk::Device device, vma::Allocator allocator,
                                const std::filesystem::path& filepath,
                                const LoadOptions& options = {})
    -> LoadedTexture;

[[nodiscard]] auto loadFromMemory(vk::Device device, vma::Allocator allocator,
                                  std::span<const std::byte> fileData,
                                  const LoadOptions& options = {})
    -> LoadedTexture;

[[nodiscard]] auto loadFromRawPixels(vk::Device device,
                                     vma::Allocator allocator,
                                     std::span<const std::byte> pixelData,
                                     int width, int height,
                                     const LoadOptions& options = {})
    -> LoadedTexture;

};  // namespace vkit::texture
