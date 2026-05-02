#pragma once

#include "vkit/graphics/buffer.hpp"
#include "vkit/graphics/enums.hpp"
#include "vkit/graphics/image.hpp"
#include "vulkan/vulkan.hpp"

namespace vkit::graphics {

auto getTextureLevelCount(int width, int height, int depth) -> int;

struct TextureCreateInfo {
  TextureType type{TextureType::k2D};
  vk::Format pixelFormat{vk::Format::eR8G8B8A8Srgb};
  vk::SampleCountFlagBits samples{vk::SampleCountFlagBits::e1};
  vk::ImageUsageFlags usage{vk::ImageUsageFlagBits::eTransferDst |
                            vk::ImageUsageFlagBits::eTransferSrc |
                            vk::ImageUsageFlagBits::eSampled |
                            vk::ImageUsageFlagBits::eStorage};
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

  [[nodiscard]] auto makeImageView(vk::Device device) const
      -> vk::UniqueImageView;
  [[nodiscard]] auto makeImageView(vk::Device device, TextureType type) const
      -> vk::UniqueImageView;
  [[nodiscard]] auto makeImageView(vk::Device device, TextureType type,
                                   std::uint32_t baseLevel,
                                   std::uint32_t levels,
                                   std::uint32_t baseLayer,
                                   std::uint32_t layers) const
      -> vk::UniqueImageView;

  [[nodiscard]] auto getImage() const -> Image;
  [[nodiscard]] auto getImageView() const -> vk::ImageView;
  [[nodiscard]] auto getSampler() const -> vk::Sampler;
  [[nodiscard]] auto getTextureType() const -> TextureType;
  [[nodiscard]] auto getPixelFormat() const -> vk::Format;
  [[nodiscard]] auto getWidth(std::uint32_t level = 0) const -> int;
  [[nodiscard]] auto getHeight(std::uint32_t level = 0) const -> int;
  [[nodiscard]] auto getDepth(std::uint32_t level = 0) const -> int;
  [[nodiscard]] auto getArrayLayerCount() const -> int;
  [[nodiscard]] auto getLevelCount() const -> int;
  [[nodiscard]] auto getSamples() const -> vk::SampleCountFlagBits;
  [[nodiscard]] auto isLayered() const -> bool;

  void setSampler(vk::Sampler);

  void recordUpload(vk::CommandBuffer cb, const Buffer& stagingBuffer);
  void recordMipmapGeneration(vk::CommandBuffer cb);

 private:
  TextureType type_{TextureType::k2D};
  vk::SampleCountFlagBits samples_{vk::SampleCountFlagBits::e1};
  bool useMipmaps_{false};

  AllocatedImage image_;

  vk::UniqueImageView view_{nullptr};
  vk::Sampler sampler_{nullptr};

  static auto createAllocatedImage(vma::Allocator allocator,
                                   const TextureCreateInfo& createInfo)
      -> AllocatedImage;
};

};  // namespace vkit::graphics
