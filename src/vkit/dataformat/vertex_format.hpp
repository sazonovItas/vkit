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

enum class AttributeFormat : uint32_t {
  kInvalid = 0,

  // --- Float32 (4 bytes) ---
  kScalarFloat32,
  kVec2Float32,
  kVec3Float32,
  kVec4Float32,
  kMat2Float32,
  kMat3Float32,
  kMat4Float32,

  // --- UInt32 (4 bytes) ---
  kScalarUInt32,
  kVec2UInt32,
  kVec3UInt32,
  kVec4UInt32,
  kMat2UInt32,
  kMat3UInt32,
  kMat4UInt32,

  // --- UInt16 (2 bytes) ---
  kScalarUInt16,
  kVec2UInt16,
  kVec3UInt16,
  kVec4UInt16,
  kMat2UInt16,
  kMat3UInt16,
  kMat4UInt16,

  // --- Int16 (2 bytes) ---
  kScalarInt16,
  kVec2Int16,
  kVec3Int16,
  kVec4Int16,
  kMat2Int16,
  kMat3Int16,
  kMat4Int16,

  // --- UInt8 (1 byte) ---
  kScalarUInt8,
  kVec2UInt8,
  kVec3UInt8,
  kVec4UInt8,
  kMat2UInt8,
  kMat3UInt8,
  kMat4UInt8,

  // --- Int8 (1 byte) ---
  kScalarInt8,
  kVec2Int8,
  kVec3Int8,
  kVec4Int8,
  kMat2Int8,
  kMat3Int8,
  kMat4Int8,
};

[[nodiscard]] auto getComponentCount(AttributeFormat format) -> std::size_t;

[[nodiscard]] auto getComponentByteSize(AttributeFormat format) -> std::size_t;

};  // namespace vkit::dataformat
