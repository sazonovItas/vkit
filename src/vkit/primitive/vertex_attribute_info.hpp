#pragma once

#include <limits>

#include "vkit/dataformat/dataformat.hpp"
#include "vkit/dataformat/vertex_format.hpp"

namespace vkit::primitive {

struct VertexAttributeInfo {
  VertexAttributeInfo() = default;
  VertexAttributeInfo(dataformat::Format vertexFormat,
                      dataformat::AttributeUsage usage);

  dataformat::Format format{dataformat::Format::eUndefined};
  dataformat::AttributeUsage usage{dataformat::AttributeUsage::kNone};
  std::size_t stride{std::numeric_limits<std::size_t>::max()};
  std::size_t size{0};
};

};  // namespace vkit::primitive
