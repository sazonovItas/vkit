#pragma once

namespace vkit::vulkan {
class CommandBlock {
 public:
  explicit CommandBlock(vk::Device device, vk::Queue queue,
                        vk::CommandPool pool);

  [[nodiscard]] auto cb() const -> vk::CommandBuffer { return *m_cb_; }

  void submit_and_wait();

 private:
  vk::Device m_device_;
  vk::Queue m_queue_;
  vk::UniqueCommandBuffer m_cb_;
};
};  // namespace vkit::vulkan
