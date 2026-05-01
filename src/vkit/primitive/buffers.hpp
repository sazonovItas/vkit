#pragma once

#include <cstddef>
#include <unordered_map>
#include <vector>

namespace vkit::primitive {

struct BufferViewInfo {
  std::size_t bufferIdx;
  std::size_t offset;
  std::size_t size;
};

class Buffers {
  friend class DeviceBuffers;

 public:
  Buffers() = default;

  [[nodiscard]] auto operator()(std::size_t bufferViewIdx) const
      -> const BufferViewInfo&;

  void addBuffer(std::size_t bufferIdx, std::vector<std::byte> data);
  void addBufferView(std::size_t bufferIdx, std::size_t offset,
                     std::size_t size);

  [[nodiscard]] auto viewCount() const noexcept -> std::size_t {
    return bufferViews_.size();
  }

  [[nodiscard]] auto getBufferBytes() const
      -> const std::unordered_map<std::size_t, std::vector<std::byte>>& {
    return bufferBytes_;
  }

 protected:
  std::unordered_map<std::size_t, std::vector<std::byte>> bufferBytes_;
  std::vector<BufferViewInfo> bufferViews_;
};

}  // namespace vkit::primitive
