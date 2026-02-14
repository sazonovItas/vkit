#pragma once

namespace lvk {
class CommandBlock {
 public:
  explicit CommandBlock(vk::Device device, vk::Queue queue,
                        vk::CommandPool command_pool);

  [[nodiscard]] auto command_buffer() const -> vk::CommandBuffer {
    return *m_command_buffer_;
  }

  void submit_and_wait();

 private:
  vk::Device m_device_;
  vk::Queue m_queue_;
  vk::UniqueCommandBuffer m_command_buffer_;
};
};  // namespace lvk
