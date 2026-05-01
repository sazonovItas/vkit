#include "vkit/dataformat/vertex_format.hpp"

namespace vkit::dataformat {

auto c_str(const AttributeUsage usage) -> const char* {
  switch (usage) {
    case AttributeUsage::kNone:
      return "none";
    case AttributeUsage::kIndex:
      return "index";
    case AttributeUsage::kPosition:
      return "position";
    case AttributeUsage::kTangent:
      return "tangent";
    case AttributeUsage::kBitangent:
      return "bitangent";
    case AttributeUsage::kNormal:
      return "normal";
    case AttributeUsage::kColor:
      return "color";
    case AttributeUsage::kJointWeights:
      return "joint_weights";
    case AttributeUsage::kJointIndices:
      return "joint_indices";
    case AttributeUsage::kTexCoord:
      return "tex_coord";
    case AttributeUsage::kCustom:
      return "custom";
    default:
      return "?";
  }
}

auto getBufferUsage(AttributeUsage usage) -> BufferUsageFlags {
  switch (usage) {
    case AttributeUsage::kIndex:
      return vk::BufferUsageFlagBits::eIndexBuffer |
             vk::BufferUsageFlagBits::eTransferDst;

    case AttributeUsage::kPosition:
    case AttributeUsage::kNormal:
    case AttributeUsage::kTangent:
    case AttributeUsage::kBitangent:
    case AttributeUsage::kColor:
    case AttributeUsage::kTexCoord:
      return vk::BufferUsageFlagBits::eVertexBuffer |
             vk::BufferUsageFlagBits::eTransferDst;
    case AttributeUsage::kJointIndices:
    case AttributeUsage::kJointWeights:
    case AttributeUsage::kCustom:
    case AttributeUsage::kNone:
    default:
      return vk::BufferUsageFlagBits::eTransferDst;
  }
}

[[nodiscard]] auto getComponentCount(AttributeFormat format) -> std::size_t {
  switch (format) {
    // Scalars
    case AttributeFormat::kScalarFloat32:
    case AttributeFormat::kScalarUInt32:
    case AttributeFormat::kScalarUInt16:
    case AttributeFormat::kScalarInt16:
    case AttributeFormat::kScalarUInt8:
    case AttributeFormat::kScalarInt8:
      return 1;

    // Vec2
    case AttributeFormat::kVec2Float32:
    case AttributeFormat::kVec2UInt32:
    case AttributeFormat::kVec2UInt16:
    case AttributeFormat::kVec2Int16:
    case AttributeFormat::kVec2UInt8:
    case AttributeFormat::kVec2Int8:
      return 2;

    // Vec3
    case AttributeFormat::kVec3Float32:
    case AttributeFormat::kVec3UInt32:
    case AttributeFormat::kVec3UInt16:
    case AttributeFormat::kVec3Int16:
    case AttributeFormat::kVec3UInt8:
    case AttributeFormat::kVec3Int8:
      return 3;

    // Vec4 & Mat2 (2x2 = 4)
    case AttributeFormat::kVec4Float32:
    case AttributeFormat::kMat2Float32:
    case AttributeFormat::kVec4UInt32:
    case AttributeFormat::kMat2UInt32:
    case AttributeFormat::kVec4UInt16:
    case AttributeFormat::kMat2UInt16:
    case AttributeFormat::kVec4Int16:
    case AttributeFormat::kMat2Int16:
    case AttributeFormat::kVec4UInt8:
    case AttributeFormat::kMat2UInt8:
    case AttributeFormat::kVec4Int8:
    case AttributeFormat::kMat2Int8:
      return 4;

    // Mat3 (3x3 = 9)
    case AttributeFormat::kMat3Float32:
    case AttributeFormat::kMat3UInt32:
    case AttributeFormat::kMat3UInt16:
    case AttributeFormat::kMat3Int16:
    case AttributeFormat::kMat3UInt8:
    case AttributeFormat::kMat3Int8:
      return 9;

    // Mat4 (4x4 = 16)
    case AttributeFormat::kMat4Float32:
    case AttributeFormat::kMat4UInt32:
    case AttributeFormat::kMat4UInt16:
    case AttributeFormat::kMat4Int16:
    case AttributeFormat::kMat4UInt8:
    case AttributeFormat::kMat4Int8:
      return 16;

    default:
      return 0;
  }
}

[[nodiscard]] auto getComponentByteSize(AttributeFormat format) -> std::size_t {
  switch (format) {
    // 4 Bytes (Float32, UInt32)
    case AttributeFormat::kScalarFloat32:
    case AttributeFormat::kVec2Float32:
    case AttributeFormat::kVec3Float32:
    case AttributeFormat::kVec4Float32:
    case AttributeFormat::kMat2Float32:
    case AttributeFormat::kMat3Float32:
    case AttributeFormat::kMat4Float32:

    case AttributeFormat::kScalarUInt32:
    case AttributeFormat::kVec2UInt32:
    case AttributeFormat::kVec3UInt32:
    case AttributeFormat::kVec4UInt32:
    case AttributeFormat::kMat2UInt32:
    case AttributeFormat::kMat3UInt32:
    case AttributeFormat::kMat4UInt32:
      return 4;

    // 2 Bytes (UInt16, Int16)
    case AttributeFormat::kScalarUInt16:
    case AttributeFormat::kVec2UInt16:
    case AttributeFormat::kVec3UInt16:
    case AttributeFormat::kVec4UInt16:
    case AttributeFormat::kMat2UInt16:
    case AttributeFormat::kMat3UInt16:
    case AttributeFormat::kMat4UInt16:

    case AttributeFormat::kScalarInt16:
    case AttributeFormat::kVec2Int16:
    case AttributeFormat::kVec3Int16:
    case AttributeFormat::kVec4Int16:
    case AttributeFormat::kMat2Int16:
    case AttributeFormat::kMat3Int16:
    case AttributeFormat::kMat4Int16:
      return 2;

    // 1 Byte (UInt8, Int8)
    case AttributeFormat::kScalarUInt8:
    case AttributeFormat::kVec2UInt8:
    case AttributeFormat::kVec3UInt8:
    case AttributeFormat::kVec4UInt8:
    case AttributeFormat::kMat2UInt8:
    case AttributeFormat::kMat3UInt8:
    case AttributeFormat::kMat4UInt8:

    case AttributeFormat::kScalarInt8:
    case AttributeFormat::kVec2Int8:
    case AttributeFormat::kVec3Int8:
    case AttributeFormat::kVec4Int8:
    case AttributeFormat::kMat2Int8:
    case AttributeFormat::kMat3Int8:
    case AttributeFormat::kMat4Int8:
      return 1;

    default:
      return 0;
  }
}

auto getIndexType(AttributeFormat format) -> vk::IndexType {
  switch (format) {
    case AttributeFormat::kScalarUInt32:
      return vk::IndexType::eUint32;
    case AttributeFormat::kScalarUInt16:
      return vk::IndexType::eUint16;
    case AttributeFormat::kScalarUInt8:
      return vk::IndexType::eUint8EXT;
    default:
      throw std::runtime_error{
          "Invalid AttributeFormat provided for Vulkan IndexType. "
          "Indices must be UInt32, UInt16, or UInt8."};
  }
}

};  // namespace vkit::dataformat
