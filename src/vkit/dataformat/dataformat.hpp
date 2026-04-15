#pragma once

namespace vkit::dataformat {

using Format = vk::Format;

[[nodiscard]] auto getComponentCount(Format format) -> std::size_t;

[[nodiscard]] auto getComponentByteSize(Format format) -> std::size_t;

[[nodiscard]] auto getPixelByteSize(Format format) -> std::size_t;

}  // namespace vkit::dataformat
