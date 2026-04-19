#include "vkit/dataformat/dataformat.hpp"

#include <stdexcept>

namespace vkit::dataformat {

auto getComponentCount(const Format format) -> std::size_t {
  switch (format) {
    case Format::eR8Uint:
    case Format::eR8Sint:
    case Format::eR8Unorm:
    case Format::eR8Snorm:
    case Format::eR8Uscaled:
    case Format::eR8Sscaled:
    case Format::eR8Srgb:

    case Format::eR16Uint:
    case Format::eR16Sint:
    case Format::eR16Sfloat:
    case Format::eR16Unorm:
    case Format::eR16Snorm:
    case Format::eR16Uscaled:
    case Format::eR16Sscaled:

    case Format::eR32Uint:
    case Format::eR32Sint:
    case Format::eR32Sfloat:

    case Format::eD16Unorm:
    case Format::eD32Sfloat:
    case Format::eS8Uint:
      return 1;

    case Format::eR8G8Uint:
    case Format::eR8G8Sint:
    case Format::eR8G8Unorm:
    case Format::eR8G8Snorm:
    case Format::eR8G8Uscaled:
    case Format::eR8G8Sscaled:
    case Format::eR8G8Srgb:

    case Format::eR16G16Uint:
    case Format::eR16G16Sint:
    case Format::eR16G16Sfloat:
    case Format::eR16G16Unorm:
    case Format::eR16G16Snorm:
    case Format::eR16G16Uscaled:
    case Format::eR16G16Sscaled:

    case Format::eR32G32Uint:
    case Format::eR32G32Sint:
    case Format::eR32G32Sfloat:

    case Format::eD16UnormS8Uint:
    case Format::eD24UnormS8Uint:
    case Format::eD32SfloatS8Uint:
      return 2;

    case Format::eR8G8B8Uint:
    case Format::eR8G8B8Sint:
    case Format::eR8G8B8Unorm:
    case Format::eR8G8B8Snorm:
    case Format::eR8G8B8Uscaled:
    case Format::eR8G8B8Sscaled:
    case Format::eR8G8B8Srgb:

    case Format::eR16G16B16Uint:
    case Format::eR16G16B16Sint:
    case Format::eR16G16B16Sfloat:
    case Format::eR16G16B16Unorm:
    case Format::eR16G16B16Snorm:
    case Format::eR16G16B16Uscaled:
    case Format::eR16G16B16Sscaled:

    case Format::eR32G32B32Uint:
    case Format::eR32G32B32Sint:
    case Format::eR32G32B32Sfloat:
      return 3;

    case Format::eR8G8B8A8Uint:
    case Format::eR8G8B8A8Sint:
    case Format::eR8G8B8A8Unorm:
    case Format::eR8G8B8A8Snorm:
    case Format::eR8G8B8A8Uscaled:
    case Format::eR8G8B8A8Sscaled:
    case Format::eR8G8B8A8Srgb:

    case Format::eR16G16B16A16Uint:
    case Format::eR16G16B16A16Sint:
    case Format::eR16G16B16A16Sfloat:
    case Format::eR16G16B16A16Unorm:
    case Format::eR16G16B16A16Snorm:
    case Format::eR16G16B16A16Uscaled:
    case Format::eR16G16B16A16Sscaled:

    case Format::eR32G32B32A32Uint:
    case Format::eR32G32B32A32Sint:
    case Format::eR32G32B32A32Sfloat:
      return 4;

    default:
      throw std::runtime_error{"Unsupported format in getComponentCount"};
  }
}

auto getComponentByteSize(Format format) -> std::size_t {
  switch (format) {
    case Format::eR8Uint:
    case Format::eR8Sint:
    case Format::eR8Unorm:
    case Format::eR8Snorm:
    case Format::eR8Uscaled:
    case Format::eR8Sscaled:
    case Format::eR8Srgb:
    case Format::eR8G8Uint:
    case Format::eR8G8Sint:
    case Format::eR8G8Unorm:
    case Format::eR8G8Snorm:
    case Format::eR8G8Uscaled:
    case Format::eR8G8Sscaled:
    case Format::eR8G8Srgb:
    case Format::eR8G8B8Uint:
    case Format::eR8G8B8Sint:
    case Format::eR8G8B8Unorm:
    case Format::eR8G8B8Snorm:
    case Format::eR8G8B8Uscaled:
    case Format::eR8G8B8Sscaled:
    case Format::eR8G8B8Srgb:
    case Format::eR8G8B8A8Uint:
    case Format::eR8G8B8A8Sint:
    case Format::eR8G8B8A8Unorm:
    case Format::eR8G8B8A8Snorm:
    case Format::eR8G8B8A8Uscaled:
    case Format::eR8G8B8A8Sscaled:
    case Format::eR8G8B8A8Srgb:
    case Format::eS8Uint:
      return 1;

    case Format::eR16Uint:
    case Format::eR16Sint:
    case Format::eR16Sfloat:
    case Format::eR16Unorm:
    case Format::eR16Snorm:
    case Format::eR16Uscaled:
    case Format::eR16Sscaled:
    case Format::eR16G16Uint:
    case Format::eR16G16Sint:
    case Format::eR16G16Sfloat:
    case Format::eR16G16Unorm:
    case Format::eR16G16Snorm:
    case Format::eR16G16Uscaled:
    case Format::eR16G16Sscaled:
    case Format::eR16G16B16Uint:
    case Format::eR16G16B16Sint:
    case Format::eR16G16B16Sfloat:
    case Format::eR16G16B16Unorm:
    case Format::eR16G16B16Snorm:
    case Format::eR16G16B16Uscaled:
    case Format::eR16G16B16Sscaled:
    case Format::eR16G16B16A16Uint:
    case Format::eR16G16B16A16Sint:
    case Format::eR16G16B16A16Sfloat:
    case Format::eR16G16B16A16Unorm:
    case Format::eR16G16B16A16Snorm:
    case Format::eR16G16B16A16Uscaled:
    case Format::eR16G16B16A16Sscaled:
    case Format::eD16Unorm:
      return 2;

    case Format::eR32Uint:
    case Format::eR32Sint:
    case Format::eR32Sfloat:
    case Format::eR32G32Uint:
    case Format::eR32G32Sint:
    case Format::eR32G32Sfloat:
    case Format::eR32G32B32Uint:
    case Format::eR32G32B32Sint:
    case Format::eR32G32B32Sfloat:
    case Format::eR32G32B32A32Uint:
    case Format::eR32G32B32A32Sint:
    case Format::eR32G32B32A32Sfloat:
    case Format::eD32Sfloat:
      return 4;

    default:
      throw std::runtime_error{"Unsupported format in getComponentByteSize"};
  }
}

auto getPixelByteSize(Format format) -> std::size_t {
  switch (format) {
    case Format::eR8Unorm:
    case Format::eR8Srgb:
      return 1;

    case Format::eR8G8Unorm:
      return 2;

    case Format::eR8G8B8Unorm:
      return 3;

    case Format::eR8G8B8A8Unorm:
    case Format::eR8G8B8A8Srgb:
      return 4;

    case Format::eR16G16B16A16Sfloat:
      return 8;

    case Format::eR32G32B32A32Sfloat:
      return 16;

    case Format::eD16Unorm:
      return 2;

    case Format::eD32Sfloat:
    case Format::eD24UnormS8Uint:
      return 4;

    default:
      return getComponentCount(format) * getComponentByteSize(format);
  }
}

};  // namespace vkit::dataformat
