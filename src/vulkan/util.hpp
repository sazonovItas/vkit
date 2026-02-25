#pragma once

namespace vkit::vulkan::util {
template <typename T, typename E>
auto contains(T flags, E bit) -> bool {
  return (flags & bit) == bit;
}

static void record_and_submit(vk::Device device, vk::Queue queue,
                              vk::CommandPool cp,
                              std::function<void(vk::CommandBuffer)>&& record);

class CommandBlock {
 public:
  explicit CommandBlock(vk::Device device, vk::Queue queue,
                        vk::CommandPool pool);

  [[nodiscard]] auto cb() const -> vk::CommandBuffer { return *cb_; }

  void submit_and_wait();

 private:
  vk::Device device_;
  vk::Queue queue_;
  vk::UniqueCommandBuffer cb_;
};
};  // namespace vkit::vulkan::util
