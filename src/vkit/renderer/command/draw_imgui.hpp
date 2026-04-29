#pragma once

#include <cstdint>
#include <vulkan/vulkan.hpp>

#include "vkit/imgui/window_imgui_host.hpp"
#include "vkit/renderer/command/command.hpp"

namespace vkit::renderer::command {

class DrawImGui : public command::Command {
 public:
  DrawImGui(imgui::WindowImguiHost& imguiHost, std::uint32_t frameIndex)
      : imguiHost_{imguiHost}, frameIndex_{frameIndex} {}

  void record(vk::CommandBuffer cb) const override {
    imguiHost_.render(frameIndex_, cb);
  }

 private:
  imgui::WindowImguiHost& imguiHost_;
  std::uint32_t frameIndex_;
};

};  // namespace vkit::renderer::command
