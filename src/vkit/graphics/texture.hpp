#pragma once

#include "vkit/dataformat/dataformat.hpp"
#include "vkit/graphics/buffer.hpp"
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
};

class Texture {
 public:
  Texture(vk::Device device, vma::Allocator allocator,
          const TextureCreateInfo& createInfo);

  [[nodiscard]] auto getImage() const -> Image;
  [[nodiscard]] auto getImageView() const -> vk::ImageView;
  [[nodiscard]] auto getSampler() const -> vk::Sampler;
  [[nodiscard]] auto getTextureType() const -> TextureType;
  [[nodiscard]] auto getPixelFormat() const -> dataformat::Format;
  [[nodiscard]] auto getWidth(std::uint32_t level = 0) const -> int;
  [[nodiscard]] auto getHeight(std::uint32_t level = 0) const -> int;
  [[nodiscard]] auto getDepth(std::uint32_t level = 0) const -> int;
  [[nodiscard]] auto getArrayLayerCount() const -> int;
  [[nodiscard]] auto getLevelCount() const -> int;
  [[nodiscard]] auto getSampleCount() const -> SampleCount;
  [[nodiscard]] auto isLayered() const -> bool;

  void setSampler(vk::Sampler);

  void recordUpload(vk::CommandBuffer cb, const Buffer& stagingBuffer);
  void recordMipmapGeneration(vk::CommandBuffer cb);

 private:
  TextureType type_{TextureType::k2D};
  SampleCount sampleCount_{SampleCount::k1};
  bool useMipmaps_{false};

  AllocatedImage image_;

  vk::UniqueImageView view_{nullptr};
  vk::Sampler sampler_{nullptr};

  static auto createAllocatedImage(vma::Allocator allocator,
                                   const TextureCreateInfo& createInfo)
      -> AllocatedImage;

  auto makeImageView(vk::Device device) const -> vk::UniqueImageView;
};

};  // namespace vkit::graphics
