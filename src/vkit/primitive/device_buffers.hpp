#pragma once

#include <cstddef>
#include <memory>
#include <unordered_map>

#include "vkit/graphics/device_buffer.hpp"
#include "vkit/primitive/buffers.hpp"

namespace vkit::primitive {

class DeviceBuffers {
 public:
  DeviceBuffers(const std::shared_ptr<Buffers>& buffers,
                vma::Allocator allocator,
                const graphics::util::RecordAndSubmitInfo& rsInfo);

  [[nodiscard]] auto getBufferAddress(std::size_t bufferViewIdx) const
      -> vk::DeviceAddress;

  [[nodiscard]] auto getBuffer(std::size_t bufferViewIdx) const
      -> const std::shared_ptr<graphics::DeviceBuffer>&;
  [[nodiscard]] auto getViewOffset(std::size_t bufferViewIdx) const
      -> std::size_t;

 private:
  std::shared_ptr<Buffers> buffers_;
  std::unordered_map<std::size_t, std::shared_ptr<graphics::DeviceBuffer>>
      deviceBuffers_;
};

};  // namespace vkit::primitive
