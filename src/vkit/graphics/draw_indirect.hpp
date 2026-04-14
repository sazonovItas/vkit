#pragma once

namespace vkit::graphics {

struct DrawPrimitivesIndirect {
  std::uint32_t count;
  std::uint32_t instanceCount;
  std::uint32_t first;
  std::uint32_t baseInstance;
};

struct DrawPrimitivesIndirectIndexed {
  std::uint32_t indexCount;
  std::uint32_t instanceCount;
  std::uint32_t firstIndex;
  std::uint32_t baseVertex;
  std::uint32_t baseInstance;
};

};  // namespace vkit::graphics
