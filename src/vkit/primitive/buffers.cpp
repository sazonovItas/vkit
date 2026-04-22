#include "vkit/primitive/buffers.hpp"

#include <cassert>

namespace vkit::primitive {

auto Buffers::operator()(std::size_t bufferViewIdx) const
    -> const BufferViewInfo& {
  assert(bufferViewIdx < bufferViews_.size() &&
         "Buffer view index out of range");

  return bufferViews_[bufferViewIdx];
}

void Buffers::addBuffer(std::size_t bufferIdx, std::vector<std::byte> data) {
  bufferBytes_[bufferIdx] = std::move(data);
}

void Buffers::addBufferView(std::size_t bufferIdx, std::size_t offset,
                            std::size_t size,
                            dataformat::AttributeUsage usage) {
  auto it = bufferBytes_.find(bufferIdx);
  assert(it != bufferBytes_.end() && "Buffer index not found");

  const auto& buffer = it->second;
  assert(offset + size <= buffer.size() && "Buffer view out of bounds");

  const std::byte* ptr = buffer.data() + offset;

  bufferViews_.push_back(BufferViewInfo{
      .data = std::span<const std::byte>(ptr, size),
      .usage = usage,
  });
}

DeviceBuffers::DeviceBuffers(const std::shared_ptr<Buffers>& buffers,
                             vma::Allocator allocator,
                             const graphics::util::RecordAndSubmitInfo& rsInfo)
    : buffers_{buffers} {
  deviceBufferViews_.reserve(buffers_->bufferViews_.size());

  for (const auto& view_info : buffers_->bufferViews_) {
    const auto& data = view_info.data;

    if (data.empty()) {
      deviceBufferViews_.push_back(nullptr);
      continue;
    }

    auto usage_flags = dataformat::getBufferUsage(view_info.usage);

    auto device_buffer = std::make_shared<graphics::DeviceBuffer>(
        allocator, rsInfo, std::from_range, data, usage_flags);

    deviceBufferViews_.push_back(std::move(device_buffer));
  }
}

auto DeviceBuffers::operator()(std::size_t bufferViewIdx) const
    -> const std::shared_ptr<graphics::DeviceBuffer>& {
  assert(bufferViewIdx < deviceBufferViews_.size() &&
         "Device buffer view index out of range");

  return deviceBufferViews_[bufferViewIdx];
}

auto DeviceBuffers::getBufferAddress(const std::size_t bufferViewIdx) const
    -> vk::DeviceAddress {
  assert(bufferViewIdx < deviceBufferViews_.size());

  return deviceBufferViews_[bufferViewIdx]->getAddress();
}

}  // namespace vkit::primitive
