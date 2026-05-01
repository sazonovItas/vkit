#pragma once

namespace vkit::dataformat {

[[nodiscard]] auto getComponentCount(vk::Format format) -> std::size_t;

[[nodiscard]] auto getComponentByteSize(vk::Format format) -> std::size_t;

[[nodiscard]] auto getPixelByteSize(vk::Format format) -> std::size_t;

};  // namespace vkit::dataformat
