#include "vkit/primitive/buffers.hpp"

#include <cassert>

#include "vkit/primitive/device_buffers.hpp"

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
                            std::size_t size) {
  auto it = bufferBytes_.find(bufferIdx);
  assert(it != bufferBytes_.end() && "Buffer index not found");
  assert(offset + size <= it->second.size() && "Buffer view out of bounds");

  bufferViews_.push_back(BufferViewInfo{
      .bufferIdx = bufferIdx,
      .offset = offset,
      .size = size,
  });
}

DeviceBuffers::DeviceBuffers(const std::shared_ptr<Buffers>& buffers,
                             vma::Allocator allocator,
                             const graphics::util::RecordAndSubmitInfo& rsInfo)
    : buffers_{buffers} {
  for (const auto& [bufferIdx, data] : buffers_->getBufferBytes()) {
    if (data.empty()) continue;

    vk::BufferUsageFlags usage = vk::BufferUsageFlagBits::eVertexBuffer |
                                 vk::BufferUsageFlagBits::eIndexBuffer |
                                 vk::BufferUsageFlagBits::eShaderDeviceAddress;

    deviceBuffers_[bufferIdx] = std::make_shared<graphics::DeviceBuffer>(
        allocator, rsInfo, std::from_range, data, usage);
  }
}

auto DeviceBuffers::getBufferAddress(std::size_t bufferViewIdx) const
    -> vk::DeviceAddress {
  const auto& view_info = buffers_->operator()(bufferViewIdx);
  auto it = deviceBuffers_.find(view_info.bufferIdx);

  if (it == deviceBuffers_.end()) {
    return 0;
  }

  return it->second->getAddress() + view_info.offset;
}

auto DeviceBuffers::getBuffer(std::size_t bufferViewIdx) const
    -> const std::shared_ptr<graphics::DeviceBuffer>& {
  const auto& view_info = buffers_->operator()(bufferViewIdx);
  auto it = deviceBuffers_.find(view_info.bufferIdx);

  if (it == deviceBuffers_.end()) {
    static std::shared_ptr<graphics::DeviceBuffer> null_buffer = nullptr;
    return null_buffer;
  }

  return it->second;
}

auto DeviceBuffers::getViewOffset(std::size_t bufferViewIdx) const
    -> std::size_t {
  return buffers_->operator()(bufferViewIdx).offset;
}

};  // namespace vkit::primitive
