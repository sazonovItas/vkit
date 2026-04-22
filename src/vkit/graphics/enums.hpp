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

enum class SampleCount { k1, k4, k8, k16, k32 };

auto getSampleCountFlagBits(SampleCount sampleCount) -> vk::SampleCountFlagBits;

};  // namespace vkit::graphics
