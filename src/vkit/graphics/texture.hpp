#pragma once

#include <stb_image.h>

#include "vkit/dataformat/dataformat.hpp"
#include "vkit/graphics/buffer.hpp"
#include "vkit/graphics/device.hpp"
#include "vkit/graphics/enums.hpp"
#include "vkit/graphics/image.hpp"

namespace vkit::graphics {

auto getTextureLevelCount(int width, int height, int depth) -> int;

struct TextureCreateInfo {
  TextureType type{TextureType::k2D};
  dataformat::Format pixelFormat{dataformat::Format::eR8G8B8A8Srgb};
  SampleCount sampleCount{SampleCount::k1};
  bool useMipmaps{false};
  int width{1};
  int height{1};
  int depth{1};
  int arrayLayerCount{1};
  int levelCount{1};
  Buffer* buffer{nullptr};
};

class Texture {
 public:
  Texture(GfxDevice& gfxDevice, const TextureCreateInfo& createInfo);

  [[nodiscard]] auto makeImageView() const -> vk::UniqueImageView;
  [[nodiscard]] auto makeImageView(std::uint32_t baseMipLevel,
                                   std::uint32_t levelCount,
                                   std::uint32_t baseArrayLayer,
                                   std::uint32_t layerCount) const
      -> vk::UniqueImageView;

  [[nodiscard]] auto getImage() const -> Image;
  [[nodiscard]] auto getTextureType() const -> TextureType;
  [[nodiscard]] auto getPixelFormat() const -> dataformat::Format;
  [[nodiscard]] auto getWidth(std::uint32_t level = 0) const -> int;
  [[nodiscard]] auto getHeight(std::uint32_t level = 0) const -> int;
  [[nodiscard]] auto getDepth(std::uint32_t level = 0) const -> int;
  [[nodiscard]] auto getArrayLayerCount() const -> int;
  [[nodiscard]] auto getLevelCount() const -> int;
  [[nodiscard]] auto getSampleCount() const -> SampleCount;
  [[nodiscard]] auto isLayered() const -> bool;

  void update(const Buffer& buffer);

  void generateMipmaps();

 private:
  const GfxDevice& device_;

  TextureType type_{TextureType::k2D};
  SampleCount sampleCount_{SampleCount::k1};
  bool useMipmaps_{false};

  AllocatedImage image_;

  static auto createAllocatedImage(GfxDevice& device,
                                   const TextureCreateInfo& createInfo)
      -> AllocatedImage;
};

};  // namespace vkit::graphics
