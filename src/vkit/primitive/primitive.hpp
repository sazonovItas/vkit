#pragma once

namespace vkit::scene {

enum class PrimitiveMode : std::uint32_t {
  kPoints = 0,
  kLines,
  kLineLoop,
  kLineStrip,
  kTriangles,
  kTriangleStrip,
  kTriangleFan,
};

class Primitive {};

};  // namespace vkit::scene
