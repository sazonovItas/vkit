#pragma once

namespace vkit::graphics {

enum class TextureType {
  k1D,
  k1DArray,
  k2D,
  k2DArray,
  kCubeMap,
  kCubeArray,
  k3D,
};

auto toVkImageType(TextureType type) -> vk::ImageType;

auto toVkImageViewType(TextureType type) -> vk::ImageViewType;

};  // namespace vkit::graphics
