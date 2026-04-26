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

struct PrimitiveData {
  AttributeData position;
  AttributeData color;
  AttributeData normal;
  AttributeData texcoords[4];
  AttributeData tangent;
  AttributeData bitangent;
  AttributeData jointIndices;
  AttributeData jointWeights;
};

static_assert(sizeof(PrimitiveData) == 264,
              "PrimitiveData must be exactly 264 bytes for std430 alignment.");

class Primitive {
 public:
  using Data = PrimitiveData;
  using VertexMode = PrimitiveVertexMode;
  using PolygonMode = PrimitivePolygonMode;

  Primitive(const std::shared_ptr<DeviceBuffers>& buffers,
            const PrimitiveAttributes& attrs);

  VertexMode vertexMode{VertexMode::eTriangleList};
  PolygonMode polygonMode{PolygonMode::eFill};
  DevicePrimitiveAttributes attrs;

  void attach(const std::shared_ptr<Attachment>& attachment);
  void detach(Attachment* attachment);

  void setVertexMode(const VertexMode mode) { vertexMode = mode; }
  void setPolygonMode(const PolygonMode mode) { polygonMode = mode; }

  [[nodiscard]] auto getData() const -> PrimitiveData {
    return PrimitiveData{
        .position = attrs.position.getData(),
        .color = attrs.color.getData(),
        .normal = attrs.normal.getData(),
        .texcoords =
            {
                attrs.texcoords[0].getData(),
                attrs.texcoords[1].getData(),
                attrs.texcoords[2].getData(),
                attrs.texcoords[3].getData(),
            },
        .tangent = attrs.tangent.getData(),
        .bitangent = attrs.bitangent.getData(),
        .jointIndices = attrs.jointIndices.getData(),
        .jointWeights = attrs.jointWeights.getData(),
    };
  }

  [[nodiscard]] auto getIndexBuffer() const
      -> const std::shared_ptr<graphics::DeviceBuffer>& {
    return (*buffers_)(attrs.index.bufferViewIdx);
  }

  [[nodiscard]] auto getPositionBuffer() const
      -> const std::shared_ptr<graphics::DeviceBuffer>& {
    return (*buffers_)(attrs.position.bufferViewIdx);
  }

  [[nodiscard]] auto getColorBuffer() const
      -> const std::shared_ptr<graphics::DeviceBuffer>& {
    return (*buffers_)(attrs.color.bufferViewIdx);
  }

  [[nodiscard]] auto getNormalBuffer() const
      -> const std::shared_ptr<graphics::DeviceBuffer>& {
    return (*buffers_)(attrs.normal.bufferViewIdx);
  }

  [[nodiscard]] auto getTexCoordBuffer(const std::size_t idx) const
      -> const std::shared_ptr<graphics::DeviceBuffer>& {
    assert(idx > 4 && "Only 4 texture coordinates are supported");
    return (*buffers_)(attrs.texcoords[idx].bufferViewIdx);
  }

  [[nodiscard]] auto getTangentBuffer() const
      -> const std::shared_ptr<graphics::DeviceBuffer>& {
    return (*buffers_)(attrs.tangent.bufferViewIdx);
  }

  [[nodiscard]] auto getBitngentBuffer() const
      -> const std::shared_ptr<graphics::DeviceBuffer>& {
    return (*buffers_)(attrs.bitangent.bufferViewIdx);
  }

  [[nodiscard]] auto getJointIndicesBuffer() const
      -> const std::shared_ptr<graphics::DeviceBuffer>& {
    return (*buffers_)(attrs.jointIndices.bufferViewIdx);
  }

  [[nodiscard]] auto getJointWeightsBuffer() const
      -> const std::shared_ptr<graphics::DeviceBuffer>& {
    return (*buffers_)(attrs.jointWeights.bufferViewIdx);
  }

 private:
  std::shared_ptr<DeviceBuffers> buffers_;
  std::vector<std::shared_ptr<Attachment>> attachments_;

  auto createDeviceAttributes(const PrimitiveAttributes& attrs) const
      -> DevicePrimitiveAttributes;
};

};  // namespace vkit::primitive
