#pragma once

namespace vkit::vulkan::cmd {

class Command {
 public:
  virtual void record(vk::CommandBuffer cb) = 0;
};

};  // namespace vkit::vulkan::cmd
