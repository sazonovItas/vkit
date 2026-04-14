#include "vkit/dataformat/vertex_format.hpp"

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

};  // namespace vkit::dataformat
