#include "vkit/graphics/util.hpp"

#include <chrono>

namespace vkit::graphics::util {

void recordAndSubmit(vk::Device device, vk::Queue queue, vk::CommandPool cp,
                     std::function<void(vk::CommandBuffer)> &&record,
                     std::chrono::nanoseconds waitTimeout) {
  auto allocate_info = vk::CommandBufferAllocateInfo{};
  allocate_info.setCommandPool(cp).setCommandBufferCount(1).setLevel(
      vk::CommandBufferLevel::ePrimary);

  auto cb =
      std::move(device.allocateCommandBuffersUnique(allocate_info).front());

  auto begin_info = vk::CommandBufferBeginInfo{};
  begin_info.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);

  cb->begin(begin_info);
  record(*cb);
  cb->end();

  auto cb_info = vk::CommandBufferSubmitInfo{}.setCommandBuffer(*cb);
  auto submit_info = vk::SubmitInfo2{}.setCommandBufferInfos(cb_info);
  auto fence = device.createFenceUnique({});

  queue.submit2(submit_info, *fence);

  auto const result =
      device.waitForFences(*fence, vk::True, waitTimeout.count());
  if (result != vk::Result::eSuccess) {
    throw std::runtime_error{"failed to submit command buffer"};
  }
}

};  // namespace vkit::graphics::util
