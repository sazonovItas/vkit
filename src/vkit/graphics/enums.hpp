#pragma once

namespace vkit::graphics {

enum class TextureType { k1D, k2D, kCubeMap, k3D };

auto getImageType(TextureType type) -> vk::ImageType;

auto getImageViewType(TextureType type) -> vk::ImageViewType;

enum class SampleCount { k1, k4, k8, k16, k32 };

auto getSampleCountFlagBits(SampleCount sampleCount) -> vk::SampleCountFlagBits;

};  // namespace vkit::graphics
