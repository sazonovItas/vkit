#include "vkit/dataformat/vertex_format.hpp"

#include "vulkan/vulkan.hpp"

namespace vkit::dataformat {

auto c_str(const AttributeUsage usage) -> const char* {
  switch (usage) {
    case AttributeUsage::kNone:
      return "none";
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
    case AttributeUsage::kJointIndices:
    case AttributeUsage::kJointWeights:
    case AttributeUsage::kCustom:
    case AttributeUsage::kNone:
    default:
      return vk::BufferUsageFlagBits::eTransferDst;
  }
}

};  // namespace vkit::dataformat
