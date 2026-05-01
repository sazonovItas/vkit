#include "vkit/graphics/enums.hpp"

namespace vkit::graphics {

auto toVkImageType(TextureType type) -> vk::ImageType {
  switch (type) {
    case TextureType::k1D:
    case TextureType::k1DArray:
      return vk::ImageType::e1D;
    case TextureType::k2D:
    case TextureType::k2DArray:
    case TextureType::kCubeMap:
    case TextureType::kCubeArray:
      return vk::ImageType::e2D;
    case TextureType::k3D:
      return vk::ImageType::e3D;
    default:
      throw std::runtime_error{"Unknown texture type"};
  }
}

auto toVkImageViewType(TextureType type) -> vk::ImageViewType {
  switch (type) {
    case TextureType::k1D:
      return vk::ImageViewType::e1D;
    case TextureType::k1DArray:
      return vk::ImageViewType::e1DArray;
    case TextureType::k2D:
      return vk::ImageViewType::e2D;
    case TextureType::k2DArray:
      return vk::ImageViewType::e2DArray;
    case TextureType::kCubeMap:
      return vk::ImageViewType::eCube;
    case TextureType::kCubeArray:
      return vk::ImageViewType::eCubeArray;
    case TextureType::k3D:
      return vk::ImageViewType::e3D;
    default:
      throw std::runtime_error{"Unknown texture type"};
  }
}

};  // namespace vkit::graphics
