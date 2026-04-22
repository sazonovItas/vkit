#include "vkit/graphics/util.hpp"

#include <chrono>

namespace vkit::graphics::util {

void recordAndSubmit(const RecordAndSubmitInfo& info,
                     std::function<void(vk::CommandBuffer)>&& record,
                     std::chrono::nanoseconds waitTimeout) {
  auto allocate_info = vk::CommandBufferAllocateInfo{};
  allocate_info.setCommandPool(info.commandPool)
      .setCommandBufferCount(1)
      .setLevel(vk::CommandBufferLevel::ePrimary);

  auto cb = std::move(
      info.device.allocateCommandBuffersUnique(allocate_info).front());

  auto begin_info = vk::CommandBufferBeginInfo{};
  begin_info.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);

  cb->begin(begin_info);
  record(*cb);
  cb->end();

  auto cb_info = vk::CommandBufferSubmitInfo{}.setCommandBuffer(*cb);
  auto submit_info = vk::SubmitInfo2{}.setCommandBufferInfos(cb_info);
  auto fence = info.device.createFenceUnique({});

  info.queue.submit2(submit_info, *fence);

  const auto result =
      info.device.waitForFences(*fence, vk::True, waitTimeout.count());
  if (result != vk::Result::eSuccess) {
    throw std::runtime_error{"Failed to submit command buffer"};
  }
}

void recordAndSubmit(const RecordAndSubmitInfo& info,
                     std::function<void(vk::CommandBuffer)>&& record,
                     vk::Fence fence) {
  auto allocate_info = vk::CommandBufferAllocateInfo{};
  allocate_info.setCommandPool(info.commandPool)
      .setCommandBufferCount(1)
      .setLevel(vk::CommandBufferLevel::ePrimary);

  auto cb = std::move(
      info.device.allocateCommandBuffersUnique(allocate_info).front());

  auto begin_info = vk::CommandBufferBeginInfo{};
  begin_info.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);

  cb->begin(begin_info);
  record(*cb);
  cb->end();

  auto cb_info = vk::CommandBufferSubmitInfo{}.setCommandBuffer(*cb);
  auto submit_info = vk::SubmitInfo2{}.setCommandBufferInfos(cb_info);

  info.queue.submit2(submit_info, fence);
}

};  // namespace vkit::graphics::util
