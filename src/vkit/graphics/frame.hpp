#pragma once

namespace vkit::graphics {

struct Frame {
  vk::Image image{};
  vk::ImageView imageView{};

  vk::Semaphore imageAvailableSemaphore{};
  vk::Semaphore renderFinishedSemaphore{};
  vk::Fence inFlightFence{};

  vk::CommandBuffer cb{};

  bool valid{false};
};

}  // namespace vkit::graphics
