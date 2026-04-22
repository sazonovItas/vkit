#pragma once

#include <limits>

#include "vkit/dataformat/dataformat.hpp"
#include "vkit/dataformat/vertex_format.hpp"

namespace vkit::primitive {

struct AttributeInfo {
  dataformat::Format format{dataformat::Format::eUndefined};
  dataformat::AttributeUsage usage{dataformat::AttributeUsage::kNone};
  std::size_t offset{std::numeric_limits<std::size_t>::max()};
  std::size_t stride{std::numeric_limits<std::size_t>::max()};
  std::size_t size{0};
  std::size_t count{0};

  auto isValid() const -> bool {
    return format != dataformat::Format::eUndefined &&
           usage != dataformat::AttributeUsage::kNone &&
           offset != std::numeric_limits<std::size_t>::max() &&
           stride != std::numeric_limits<std::size_t>::max() && count != 0 &&
           size != 0;
  }
};

struct Attribute {
  AttributeInfo info;
  std::size_t bufferViewIdx{std::numeric_limits<std::size_t>::max()};

  virtual auto isValid() const -> bool {
    return info.isValid() &&
           bufferViewIdx != std::numeric_limits<std::size_t>::max();
  }
};

struct DeviceAttribute {
  AttributeInfo info{};
  vk::DeviceAddress address{0};
  std::size_t bufferViewIdx{0};

  DeviceAttribute() = default;

  DeviceAttribute(const Attribute& attr, vk::DeviceAddress addr)
      : info(attr.info), address(addr), bufferViewIdx(attr.bufferViewIdx) {}

  bool isValid() const { return info.isValid() && address != 0; }
};

};  // namespace vkit::primitive
