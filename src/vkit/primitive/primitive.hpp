#pragma once

#include <memory>
#include <optional>
#include <vector>

#include "vkit/dataformat/vertex_format.hpp"
#include "vkit/item/storage_item.hpp"
#include "vkit/material/material.hpp"
#include "vkit/primitive/buffers.hpp"
#include "vkit/primitive/device_buffers.hpp"
#include "vkit/primitive/enums.hpp"
#include "vkit/primitive/primitive_attachment.hpp"
#include "vkit/primitive/vertex_attribute.hpp"

namespace vkit::primitive {

struct PrimitiveAttributes {
  Attribute index;
  Attribute position;
  Attribute color;
  Attribute normal;
  std::array<Attribute, 4> texcoords;
  Attribute tangent;
  Attribute bitangent;
  std::array<Attribute, 2> jointIndices{};
  std::array<Attribute, 2> jointWeights{};
};

struct DevicePrimitiveAttributes {
  DeviceAttribute index;
  DeviceAttribute position;
  DeviceAttribute color;
  DeviceAttribute normal;
  std::array<DeviceAttribute, 4> texcoords;
  DeviceAttribute tangent;
  DeviceAttribute bitangent;
  std::array<DeviceAttribute, 2> jointIndices{};
  std::array<DeviceAttribute, 2> jointWeights{};
};

struct PrimitiveData {
  AttributeData position;
  AttributeData color;
  AttributeData normal;
  std::array<AttributeData, 4> texcoords;
  AttributeData tangent;
  AttributeData bitangent;
  std::array<AttributeData, 2> jointIndices{};
  std::array<AttributeData, 2> jointWeights{};
};

static_assert(sizeof(PrimitiveData) == 312,
              "PrimitiveData must be exactly 312 bytes for std430 alignment.");

class Primitive : public StorageItem {
 public:
  using Data = PrimitiveData;
  using VertexMode = PrimitiveVertexMode;
  using PolygonMode = PrimitivePolygonMode;

  Primitive(const std::shared_ptr<DeviceBuffers>& buffers,
            const PrimitiveAttributes& attrs);

  VertexMode vertexMode{VertexMode::eTriangleList};
  PolygonMode polygonMode{PolygonMode::eFill};
  DevicePrimitiveAttributes attrs;

  [[nodiscard]] auto getMaterialType() const -> material::Type {
    return materialType_;
  }
  void setMaterialType(const material::Type type) { materialType_ = type; }

  [[nodiscard]] auto getMaterialId() const -> std::uint32_t {
    return materialId_;
  }
  void setMaterialId(const std::uint32_t id) { materialId_ = id; }

  template <typename T>
  [[nodiscard]] auto getAttachmentAs() const -> std::shared_ptr<T> {
    for (const auto& attachment : attachments_) {
      if (auto casted = std::dynamic_pointer_cast<T>(attachment)) {
        return casted;
      }
    }
    return nullptr;
  }

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
        .jointIndices =
            {
                attrs.jointIndices[0].getData(),
                attrs.jointIndices[1].getData(),
            },
        .jointWeights =
            {
                attrs.jointWeights[0].getData(),
                attrs.jointWeights[1].getData(),
            },
    };
  }

  [[nodiscard]] auto getIndexBuffer() const
      -> const std::shared_ptr<graphics::DeviceBuffer>& {
    return buffers_->getBuffer(attrs.index.bufferViewIdx);
  }

  [[nodiscard]] auto getIndexBufferOffset() const -> std::size_t {
    return buffers_->getViewOffset(attrs.index.bufferViewIdx) +
           attrs.index.info.offset;
  }

  [[nodiscard]] auto getIndexType() const -> vk::IndexType {
    return dataformat::getIndexType(attrs.index.info.format);
  }

 private:
  std::optional<std::uint32_t> id_;
  std::shared_ptr<DeviceBuffers> buffers_;
  std::vector<std::shared_ptr<Attachment>> attachments_;

  material::Type materialType_{material::Type::kNone};
  std::uint32_t materialId_{0};

  [[nodiscard]] static auto createDeviceAttributes(
      const std::shared_ptr<DeviceBuffers>& buffers,
      const PrimitiveAttributes& attrs) -> DevicePrimitiveAttributes;
};

};  // namespace vkit::primitive
