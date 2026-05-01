#include "vkit/primitive/primitive.hpp"

#include <ranges>

namespace vkit::primitive {

Primitive::Primitive(const std::shared_ptr<DeviceBuffers>& buffers,
                     const PrimitiveAttributes& attrs)
    : attrs{createDeviceAttributes(buffers, attrs)}, buffers_{buffers} {}

void Primitive::attach(const std::shared_ptr<Attachment>& attachment) {
  if (attachment) {
    attachment->setPrimitive(this);
    attachments_.push_back(attachment);
  }
}

void Primitive::detach(Attachment* attachment) {
  std::erase_if(attachments_,
                [attachment](const auto& a) { return a.get() == attachment; });
}

auto Primitive::createDeviceAttributes(
    const std::shared_ptr<DeviceBuffers>& buffers,
    const PrimitiveAttributes& attrs) -> DevicePrimitiveAttributes {
  DevicePrimitiveAttributes device_attrs{};

  auto resolve = [&buffers](const Attribute& attr) -> DeviceAttribute {
    if (!attr.isValid()) {
      return DeviceAttribute{};
    }
    return DeviceAttribute{attr, buffers->getBufferAddress(attr.bufferViewIdx)};
  };

  device_attrs.index = resolve(attrs.index);
  device_attrs.position = resolve(attrs.position);
  device_attrs.color = resolve(attrs.color);
  device_attrs.normal = resolve(attrs.normal);
  device_attrs.tangent = resolve(attrs.tangent);
  device_attrs.bitangent = resolve(attrs.bitangent);

  std::ranges::transform(attrs.texcoords, device_attrs.texcoords.begin(),
                         resolve);
  std::ranges::transform(attrs.jointIndices, device_attrs.jointIndices.begin(),
                         resolve);
  std::ranges::transform(attrs.jointWeights, device_attrs.jointWeights.begin(),
                         resolve);

  return device_attrs;
}

};  // namespace vkit::primitive
