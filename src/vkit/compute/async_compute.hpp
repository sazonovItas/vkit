#pragma once

#include <memory>
#include <vector>

#include "vkit/graphics/command.hpp"
#include "vulkan/vulkan.hpp"

namespace vkit::compute {

struct ComputeTask {
  std::vector<std::unique_ptr<graphics::Command>> commands;

  template <typename T, typename... Args>
  auto add(Args&&... args) -> ComputeTask& {
    commands.push_back(std::make_unique<T>(std::forward<Args>(args)...));
    return *this;
  }
};

struct ComputeResult {
  vk::UniqueCommandBuffer commadBuffer;
  vk::UniqueSemaphore finishedSemaphore;
};

class AsyncCompute {
 public:
  AsyncCompute(vk::Device device, vk::Queue computeQueue,
               std::uint32_t queueFamilyIndex);
  ~AsyncCompute() = default;

  [[nodiscard]] auto submit(const ComputeTask& task,
                            vk::Fence computeFence = nullptr) -> ComputeResult;

 private:
  vk::Device device_;
  vk::Queue queue_;
  vk::UniqueCommandPool commandPool_;
};

};  // namespace vkit::compute
