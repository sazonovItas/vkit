#include "vkit/texture/loader.hpp"

#include <stb_image.h>

#include <cstring>
#include <format>
#include <stdexcept>

namespace vkit::texture {

auto loadFromRawPixels(vk::Device device, vma::Allocator allocator,
                       std::span<const std::byte> pixelData, int width,
                       int height, const LoadOptions& options)
    -> LoadedTexture {
  std::size_t bytes_per_channel = options.isHdr ? sizeof(float) : 1;
  auto image_size_bytes = static_cast<std::size_t>(width) *
                          static_cast<std::size_t>(height) * 4 *
                          bytes_per_channel;

  if (pixelData.size_bytes() != image_size_bytes) {
    throw std::runtime_error(
        "Raw pixel data size does not match expected width * height * 4 * "
        "channel_size");
  }

  graphics::MappedBuffer staging_buffer{allocator, std::from_range, pixelData,
                                        vk::BufferUsageFlagBits::eTransferSrc};

  graphics::TextureCreateInfo create_info{};
  create_info.type = options.type;
  create_info.width = width;
  create_info.height = height;
  create_info.depth = 1;
  create_info.arrayLayerCount = 1;
  create_info.useMipmaps = options.useMipmaps;

  if (options.isHdr) {
    create_info.pixelFormat = dataformat::Format::eR32G32B32A32Sfloat;
  } else {
    create_info.pixelFormat = options.isSrgb
                                  ? dataformat::Format::eR8G8B8A8Srgb
                                  : dataformat::Format::eR8G8B8A8Unorm;
  }

  create_info.levelCount =
      options.useMipmaps ? graphics::getTextureLevelCount(width, height, 1) : 1;

  auto texture =
      std::make_shared<graphics::Texture>(device, allocator, create_info);

  return LoadedTexture{
      .texture = std::move(texture),
      .stagingBuffer = std::move(staging_buffer),
  };
}

auto loadFromFile(vk::Device device, vma::Allocator allocator,
                  const std::filesystem::path& filepath,
                  const LoadOptions& options) -> LoadedTexture {
  int width;
  int height;
  int channels;
  void* pixels = nullptr;
  std::size_t image_size = 0;

  LoadOptions final_options = options;

  if (stbi_is_hdr(filepath.string().c_str()) != 0) {
    pixels = stbi_loadf(filepath.string().c_str(), &width, &height, &channels,
                        STBI_rgb_alpha);
    image_size = static_cast<std::size_t>(width * height * 4) * sizeof(float);
    final_options.isHdr = true;
    final_options.isSrgb = false;
  } else {
    pixels = stbi_load(filepath.string().c_str(), &width, &height, &channels,
                       STBI_rgb_alpha);
    image_size = static_cast<size_t>(width) * static_cast<size_t>(height) * 4;
    final_options.isHdr = false;
  }

  if (!pixels) {
    throw std::runtime_error(
        std::format("Failed to load texture file: {}", filepath.string()));
  }

  std::span<const std::byte> pixel_span{
      reinterpret_cast<const std::byte*>(pixels), image_size};

  auto result = loadFromRawPixels(device, allocator, pixel_span, width, height,
                                  final_options);

  stbi_image_free(pixels);

  return result;
}

auto loadFromMemory(vk::Device device, vma::Allocator allocator,
                    std::span<const std::byte> fileData,
                    const LoadOptions& options) -> LoadedTexture {
  int width;
  int height;
  int channels;
  void* pixels = nullptr;
  std::size_t image_size = 0;

  LoadOptions final_options = options;

  const auto* raw_data = reinterpret_cast<const stbi_uc*>(fileData.data());
  int raw_size = static_cast<int>(fileData.size_bytes());

  if (stbi_is_hdr_from_memory(raw_data, raw_size)) {
    pixels = stbi_loadf_from_memory(raw_data, raw_size, &width, &height,
                                    &channels, STBI_rgb_alpha);
    image_size = static_cast<std::size_t>(width * height * 4) * sizeof(float);
    final_options.isHdr = true;
    final_options.isSrgb = false;
  } else {
    pixels = stbi_load_from_memory(raw_data, raw_size, &width, &height,
                                   &channels, STBI_rgb_alpha);
    image_size =
        static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4;
    final_options.isHdr = false;
  }

  if (!pixels) {
    throw std::runtime_error("Failed to decode texture from memory span");
  }

  std::span<const std::byte> pixel_span{
      reinterpret_cast<const std::byte*>(pixels), image_size};

  auto result = loadFromRawPixels(device, allocator, pixel_span, width, height,
                                  final_options);

  stbi_image_free(pixels);

  return result;
}

}  // namespace vkit::texture
