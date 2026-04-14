#include "vkit/dataformat/dataformat.hpp"

#include <stdexcept>

namespace vkit::dataformat {

auto getComponentCount(const Format format) -> std::size_t {
  switch (format) {
    case Format::eR8Uint:
    case Format::eR8Srgb:
    case Format::eR8Unorm:
    case Format::eR8Snorm:
    case Format::eR8Uscaled:
    case Format::eR8Sscaled:

    case Format::eR16Uint:
    case Format::eR16Sfloat:
    case Format::eR16Unorm:
    case Format::eR16Snorm:
    case Format::eR16Uscaled:
    case Format::eR16Sscaled:

    case Format::eR32Sfloat:
    case Format::eR32Uint:
      return 1;

    case Format::eR8G8Srgb:
    case Format::eR8G8Uint:
    case Format::eR8G8Unorm:
    case Format::eR8G8Snorm:
    case Format::eR8G8Uscaled:
    case Format::eR8G8Sscaled:

    case Format::eR16G16Uint:
    case Format::eR16G16Sfloat:
    case Format::eR16G16Unorm:
    case Format::eR16G16Snorm:
    case Format::eR16G16Uscaled:
    case Format::eR16G16Sscaled:

    case Format::eR32G32Sfloat:
    case Format::eR32G32Uint:
      return 2;

    case Format::eR8G8B8Uint:
    case Format::eR8G8B8Srgb:
    case Format::eR8G8B8Unorm:
    case Format::eR8G8B8Snorm:
    case Format::eR8G8B8Uscaled:
    case Format::eR8G8B8Sscaled:

    case Format::eR16G16B16Uint:
    case Format::eR16G16B16Sfloat:
    case Format::eR16G16B16Unorm:
    case Format::eR16G16B16Snorm:
    case Format::eR16G16B16Uscaled:
    case Format::eR16G16B16Sscaled:

    case Format::eR32G32B32Uint:
    case Format::eR32G32B32Sfloat:
      return 3;

    case Format::eR8G8B8A8Uint:
    case Format::eR8G8B8A8Srgb:
    case Format::eR8G8B8A8Unorm:
    case Format::eR8G8B8A8Snorm:
    case Format::eR8G8B8A8Uscaled:
    case Format::eR8G8B8A8Sscaled:

    case Format::eR16G16B16A16Uint:
    case Format::eR16G16B16A16Sfloat:
    case Format::eR16G16B16A16Unorm:
    case Format::eR16G16B16A16Snorm:
    case Format::eR16G16B16A16Uscaled:
    case Format::eR16G16B16A16Sscaled:

    case Format::eR32G32B32A32Uint:
    case Format::eR32G32B32A32Sfloat:
      return 4;

    default:
      throw std::runtime_error{"bad data format"};
  }
}

auto getComponentByteSize(Format format) -> std::size_t {
  switch (format) {
    case Format::eR8Uint:
    case Format::eR8Srgb:
    case Format::eR8Unorm:
    case Format::eR8Snorm:
    case Format::eR8Uscaled:
    case Format::eR8Sscaled:

    case Format::eR8G8Srgb:
    case Format::eR8G8Uint:
    case Format::eR8G8Unorm:
    case Format::eR8G8Snorm:
    case Format::eR8G8Uscaled:
    case Format::eR8G8Sscaled:

    case Format::eR8G8B8Uint:
    case Format::eR8G8B8Srgb:
    case Format::eR8G8B8Unorm:
    case Format::eR8G8B8Snorm:
    case Format::eR8G8B8Uscaled:
    case Format::eR8G8B8Sscaled:

    case Format::eR8G8B8A8Uint:
    case Format::eR8G8B8A8Srgb:
    case Format::eR8G8B8A8Unorm:
    case Format::eR8G8B8A8Snorm:
    case Format::eR8G8B8A8Uscaled:
    case Format::eR8G8B8A8Sscaled:
      return 1;

    case Format::eR16Uint:
    case Format::eR16Sfloat:
    case Format::eR16Unorm:
    case Format::eR16Snorm:
    case Format::eR16Uscaled:
    case Format::eR16Sscaled:

    case Format::eR16G16Uint:
    case Format::eR16G16Sfloat:
    case Format::eR16G16Unorm:
    case Format::eR16G16Snorm:
    case Format::eR16G16Uscaled:
    case Format::eR16G16Sscaled:

    case Format::eR16G16B16Uint:
    case Format::eR16G16B16Sfloat:
    case Format::eR16G16B16Unorm:
    case Format::eR16G16B16Snorm:
    case Format::eR16G16B16Uscaled:
    case Format::eR16G16B16Sscaled:

    case Format::eR16G16B16A16Uint:
    case Format::eR16G16B16A16Sfloat:
    case Format::eR16G16B16A16Unorm:
    case Format::eR16G16B16A16Snorm:
    case Format::eR16G16B16A16Uscaled:
    case Format::eR16G16B16A16Sscaled:
      return 2;

    case Format::eR32Sfloat:
    case Format::eR32Uint:

    case Format::eR32G32Sfloat:
    case Format::eR32G32Uint:

    case Format::eR32G32B32Uint:
    case Format::eR32G32B32Sfloat:

    case Format::eR32G32B32A32Uint:
    case Format::eR32G32B32A32Sfloat:
      return 4;

    default:
      throw std::runtime_error{"bad data format"};
  }
}

};  // namespace vkit::dataformat
