#pragma once

#include <memory>

#include "vkit/primitive/buffers.hpp"
#include "vkit/primitive/enums.hpp"
#include "vkit/primitive/vertex_attribute.hpp"

namespace vkit::primitive {

struct PrimitiveAttributes {
  Attribute index;
  Attribute position;
  Attribute color;
  Attribute normal;
  Attribute texcoords[4];
  Attribute tangent;
  Attribute bitangent;
  Attribute jointIndices;
  Attribute jointWeights;
};

struct DevicePrimitiveAttributes {
  DeviceAttribute index;
  DeviceAttribute position;
  DeviceAttribute color;
  DeviceAttribute normal;
  DeviceAttribute texcoords[4];
  DeviceAttribute tangent;
  DeviceAttribute bitangent;
  DeviceAttribute jointIndices;
  DeviceAttribute jointWeights;
};

class Primitive {
 public:
  using Mode = PrimitiveVertexMode;

  Primitive(std::shared_ptr<DeviceBuffers> buffers, Mode mode,
            const PrimitiveAttributes& attrs);

  Mode mode;
  DevicePrimitiveAttributes attrs;

  auto getIndexBuffer() const
      -> const std::shared_ptr<graphics::DeviceBuffer>& {
    return (*buffers_)(attrs.index.bufferViewIdx);
  }

 private:
  std::shared_ptr<DeviceBuffers> buffers_;

  auto createDeviceAttributes(const PrimitiveAttributes& attrs) const
      -> DevicePrimitiveAttributes;
};

};  // namespace vkit::primitive
