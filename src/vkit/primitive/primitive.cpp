#include "vkit/primitive/primitive.hpp"

namespace vkit::primitive {

Primitive::Primitive(const std::shared_ptr<DeviceBuffers>& buffers,
                     VertexMode mode, const PrimitiveAttributes& attrs)
    : buffers_{buffers},
      vertexMode{mode},
      attrs{createDeviceAttributes(attrs)} {}

auto Primitive::createDeviceAttributes(const PrimitiveAttributes& attrs) const
    -> DevicePrimitiveAttributes {
  DevicePrimitiveAttributes out{};

  auto convert = [&](const Attribute& src) -> DeviceAttribute {
    if (!src.isValid()) return {};
    return DeviceAttribute{
        src,
        buffers_->getBufferAddress(src.bufferViewIdx),
    };
  };

  out.index = convert(attrs.index);
  out.position = convert(attrs.position);
  out.color = convert(attrs.color);
  out.normal = convert(attrs.normal);
  out.tangent = convert(attrs.tangent);
  out.bitangent = convert(attrs.bitangent);
  out.jointIndices = convert(attrs.jointIndices);
  out.jointWeights = convert(attrs.jointWeights);

  for (int i = 0; i < 4; ++i) {
    out.texcoords[i] = convert(attrs.texcoords[i]);
  }

  return out;
}

}  // namespace vkit::primitive
