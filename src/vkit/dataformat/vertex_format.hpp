#pragma once

namespace vkit::dataformat {

enum class AttributeUsage : uint32_t {
  kNone = 0,
  kPosition = 1,
  kTangent = 2,
  kBitangent = 3,
  kNormal = 4,
  kColor = 5,
  kJointIndices = 6,
  kJointWeights = 7,
  kTexCoord = 8,
  kCustom = 9
};

[[nodiscard]] auto c_str(AttributeUsage usage) -> const char*;

};  // namespace vkit::dataformat
