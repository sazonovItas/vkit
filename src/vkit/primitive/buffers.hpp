#pragma once

#include <cstddef>
#include <memory>
#include <span>
#include <unordered_map>
#include <vector>

#include "vkit/dataformat/vertex_format.hpp"
#include "vkit/graphics/device_buffer.hpp"

namespace vkit::primitive {

class DeviceBuffers;

struct BufferViewInfo {
  std::span<const std::byte> data;
  dataformat::AttributeUsage usage;
};

class Buffers {
  friend class DeviceBuffers;

 public:
  Buffers() = default;

  [[nodiscard]] auto operator()(std::size_t bufferViewIdx) const
      -> const BufferViewInfo&;

  void addBuffer(std::size_t bufferIdx, std::vector<std::byte> data);

  void addBufferView(std::size_t bufferIdx, std::size_t offset,
                     std::size_t size, dataformat::AttributeUsage usage);

  [[nodiscard]] std::size_t viewCount() const noexcept {
    return bufferViews_.size();
  }

 protected:
  std::unordered_map<std::size_t, std::vector<std::byte>> bufferBytes_;
  std::vector<BufferViewInfo> bufferViews_;
};

class DeviceBuffers {
 public:
  DeviceBuffers(const std::shared_ptr<Buffers>& buffers,
                vma::Allocator allocator,
                const graphics::util::RecordAndSubmitInfo& rsInfo);

  [[nodiscard]] auto operator()(std::size_t bufferViewIdx) const
      -> const std::shared_ptr<graphics::DeviceBuffer>&;

  [[nodiscard]] auto getBufferAddress(std::size_t bufferViewIdx) const
      -> vk::DeviceAddress;

  [[nodiscard]] std::size_t size() const noexcept {
    return deviceBufferViews_.size();
  }

 private:
  std::shared_ptr<Buffers> buffers_;
  std::vector<std::shared_ptr<graphics::DeviceBuffer>> deviceBufferViews_;
};

};  // namespace vkit::primitive
