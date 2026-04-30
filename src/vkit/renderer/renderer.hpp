#pragma once

#include <cassert>
#include <cstdint>
#include <memory>
#include <vector>

#include "vkit/graphics/command.hpp"
#include "vkit/graphics/device.hpp"

namespace vkit::renderer {

struct RenderTask {
  std::vector<std::unique_ptr<graphics::Command>> commands;

  template <typename T, typename... Args>
  auto add(Args&&... args) -> RenderTask& {
    static_assert(std::is_base_of_v<graphics::Command, T>,
                  "T must inherit from command::Command");

    commands.push_back(std::make_unique<T>(std::forward<Args>(args)...));
    return *this;
  }
};

struct RenderResult {
  vk::Semaphore renderFinishedSemaphore;
};

class Renderer {
 public:
  struct Frame {
    vk::UniqueCommandBuffer cb;
    vk::UniqueSemaphore renderFinished;
    vk::UniqueFence inFlightFence;
  };

  Renderer(vk::Device device, vk::Queue graphicsQueue,
           std::uint32_t queueFamilyIndex, std::uint32_t framesInFlight = 2);
  virtual ~Renderer() = default;

  Renderer(const Renderer&) = delete;
  auto operator=(const Renderer&) -> Renderer& = delete;

  Renderer(Renderer&&) = default;
  auto operator=(Renderer&&) -> Renderer& = default;

  void beginFrame(std::uint32_t frameIndex);

  [[nodiscard]] auto submit(
      std::uint32_t frameIndex, const RenderTask& task,
      vk::Semaphore waitSemaphore = nullptr,
      vk::PipelineStageFlags wait_stage =
          vk::PipelineStageFlagBits::eColorAttachmentOutput) -> RenderResult;

  [[nodiscard]] auto getFramesInFlight() const -> std::uint32_t {
    return framesInFlight_;
  }

  [[nodiscard]] auto isFrameStarted() const -> bool { return isFrameStarted_; }

 protected:
  vk::Device device_;
  vk::Queue graphicsQueue_;
  vk::UniqueCommandPool commandPool_;

  std::uint32_t framesInFlight_;

  std::vector<Frame> frames_;

 private:
  bool isFrameStarted_{false};

  graphics::DeviceWaiter waiter_;
};

};  // namespace vkit::renderer
