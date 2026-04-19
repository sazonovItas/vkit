#include "vkit/graphics/device_buffer.hpp"

#include "vkit/graphics/device.hpp"

namespace vkit::graphics {

void DeviceBuffer::copy(const GfxDevice& gfxDevice, vk::Buffer srcBuffer,
                        std::size_t srcOffset, std::size_t dstOffset,
                        vk::DeviceSize size) {
  auto copy_buffer_fn = [&](vk::CommandBuffer cb) {
    auto buffer_copy = vk::BufferCopy2{};
    buffer_copy.setSize(size).setSrcOffset(srcOffset).setDstOffset(dstOffset);
    auto copy_buffer_info = vk::CopyBufferInfo2{}
                                .setSrcBuffer(srcBuffer)
                                .setDstBuffer(buffer)
                                .setRegions(buffer_copy);

    cb.copyBuffer2(copy_buffer_info);
  };

  util::recordAndSubmit(gfxDevice.get(), gfxDevice.queues.transfer,
                        gfxDevice.getTransferCommandPool(), copy_buffer_fn);
}

};  // namespace vkit::graphics
