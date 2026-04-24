#pragma once

#include <memory>

#include "vkit/primitive/buffers.hpp"
#include "vkit/primitive/enums.hpp"
#include "vkit/primitive/primitive_attachment.hpp"
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
  using VertexMode = PrimitiveVertexMode;
  using PolygonMode = PrimitivePolygonMode;

  Primitive(const std::shared_ptr<DeviceBuffers>& buffers,
            VertexMode vertexMode, const PrimitiveAttributes& attrs);

  VertexMode vertexMode;
  DevicePrimitiveAttributes attrs;

  void attach(const std::shared_ptr<PrimitiveAttachment>& attachment);
  void detach(PrimitiveAttachment* attachment);

  auto getIndexBuffer() const
      -> const std::shared_ptr<graphics::DeviceBuffer>& {
    return (*buffers_)(attrs.index.bufferViewIdx);
  }

  auto getPositionBuffer() const
      -> const std::shared_ptr<graphics::DeviceBuffer>& {
    return (*buffers_)(attrs.position.bufferViewIdx);
  }

  auto getColorBuffer() const
      -> const std::shared_ptr<graphics::DeviceBuffer>& {
    return (*buffers_)(attrs.color.bufferViewIdx);
  }

  auto getNormalBuffer() const
      -> const std::shared_ptr<graphics::DeviceBuffer>& {
    return (*buffers_)(attrs.normal.bufferViewIdx);
  }

  auto getTexCoordBuffer(const std::size_t idx) const
      -> const std::shared_ptr<graphics::DeviceBuffer>& {
    assert(idx > 4 && "Only 4 texture coordinates are supported");
    return (*buffers_)(attrs.texcoords[idx].bufferViewIdx);
  }

  auto getTangentBuffer() const
      -> const std::shared_ptr<graphics::DeviceBuffer>& {
    return (*buffers_)(attrs.tangent.bufferViewIdx);
  }

  auto getBitngentBuffer() const
      -> const std::shared_ptr<graphics::DeviceBuffer>& {
    return (*buffers_)(attrs.bitangent.bufferViewIdx);
  }

  auto getJointIndicesBuffer() const
      -> const std::shared_ptr<graphics::DeviceBuffer>& {
    return (*buffers_)(attrs.jointIndices.bufferViewIdx);
  }

  auto getJointWeightsBuffer() const
      -> const std::shared_ptr<graphics::DeviceBuffer>& {
    return (*buffers_)(attrs.jointWeights.bufferViewIdx);
  }

 private:
  std::shared_ptr<DeviceBuffers> buffers_;
  std::vector<std::shared_ptr<PrimitiveAttachment>> attachments_;

  auto createDeviceAttributes(const PrimitiveAttributes& attrs) const
      -> DevicePrimitiveAttributes;
};

};  // namespace vkit::primitive
