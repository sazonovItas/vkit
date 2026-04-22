#pragma once

namespace vkit::dataformat {

enum class AttributeUsage : std::uint32_t {
  kNone = 0,
  kIndex = 1,
  kPosition = 2,
  kTangent = 3,
  kBitangent = 4,
  kNormal = 5,
  kColor = 6,
  kJointIndices = 7,
  kJointWeights = 8,
  kTexCoord = 9,
  kCustom = 10
};

[[nodiscard]] auto c_str(AttributeUsage usage) -> const char*;

using BufferUsageFlags = vk::Flags<vk::BufferUsageFlagBits>;

[[nodiscard]] auto getBufferUsage(AttributeUsage usage) -> BufferUsageFlags;

};  // namespace vkit::dataformat
