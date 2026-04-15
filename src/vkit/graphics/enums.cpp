#include "vkit/graphics/enums.hpp"

namespace vkit::graphics {

auto getImageType(TextureType type) -> vk::ImageType {
  switch (type) {
    case TextureType::k1D:
      return vk::ImageType::e1D;
    case TextureType::k2D:
    case TextureType::kCubeMap:
      return vk::ImageType::e2D;
    case TextureType::k3D:
      return vk::ImageType::e3D;
    default:
      throw std::runtime_error{"unknown texture type"};
  }
}

auto getImageViewType(TextureType type) -> vk::ImageViewType {
  switch (type) {
    case TextureType::k1D:
      return vk::ImageViewType::e1D;
    case TextureType::k2D:
      return vk::ImageViewType::e2D;
    case TextureType::kCubeMap:
      return vk::ImageViewType::eCube;
    case TextureType::k3D:
      return vk::ImageViewType::e3D;
    default:
      throw std::runtime_error{"unknown texture type"};
  }
}

auto getSampleCountFlagBits(SampleCount sampleCount)
    -> vk::SampleCountFlagBits {
  switch (sampleCount) {
    case SampleCount::k1:
      return vk::SampleCountFlagBits::e1;
    case SampleCount::k4:
      return vk::SampleCountFlagBits::e4;
    case SampleCount::k8:
      return vk::SampleCountFlagBits::e8;
    case SampleCount::k16:
      return vk::SampleCountFlagBits::e16;
    case SampleCount::k32:
      return vk::SampleCountFlagBits::e32;
    default:
      throw std::runtime_error{"unknown count of the samples"};
  }
}

};  // namespace vkit::graphics
