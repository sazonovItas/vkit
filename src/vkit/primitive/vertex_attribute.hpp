#pragma once

#include <cstdint>
#include <limits>

#include "vkit/dataformat/vertex_format.hpp"

namespace vkit::primitive {

struct AttributeInfo {
  dataformat::AttributeFormat format{dataformat::AttributeFormat::kInvalid};
  std::uint32_t offset{std::numeric_limits<std::uint32_t>::max()};
  std::uint32_t stride{std::numeric_limits<std::uint32_t>::max()};
  std::uint32_t count{0};

  [[nodiscard]] auto isValid() const -> bool {
    return format != dataformat::AttributeFormat::kInvalid &&
           offset != std::numeric_limits<std::uint32_t>::max() &&
           stride != std::numeric_limits<std::uint32_t>::max() && count != 0;
  }
};

struct Attribute {
  AttributeInfo info;
  std::size_t bufferViewIdx{std::numeric_limits<std::size_t>::max()};

  [[nodiscard]] virtual auto isValid() const -> bool {
    return info.isValid() &&
           bufferViewIdx != std::numeric_limits<std::size_t>::max();
  }
};

struct AttributeData {
  std::uint64_t address{0};
  std::uint32_t count{0};
  std::uint32_t offset{0};
  std::uint32_t stride{0};
  std::uint32_t format{0};
};

static_assert(sizeof(AttributeData) == 24,
              "AttributeData must be exactly 24 bytes for GLSL alignment.");

struct DeviceAttribute {
  AttributeInfo info{};
  vk::DeviceAddress address{0};
  std::size_t bufferViewIdx{0};

  DeviceAttribute() = default;

  DeviceAttribute(const Attribute& attr, vk::DeviceAddress addr)
      : info(attr.info), address(addr), bufferViewIdx(attr.bufferViewIdx) {}

  [[nodiscard]] bool isValid() const { return info.isValid() && address != 0; }

  [[nodiscard]] auto getData() const -> AttributeData {
    AttributeData data{};
    if (!isValid()) return data;

    data.address = static_cast<std::uint64_t>(address);
    data.count = info.count;
    data.offset = info.offset;
    data.stride = info.stride;
    data.format = static_cast<std::uint32_t>(info.format);

    return data;
  }
};

}  // namespace vkit::primitive
