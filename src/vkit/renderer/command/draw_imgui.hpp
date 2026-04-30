#pragma once

#include <cstdint>

#include "vkit/graphics/command.hpp"
#include "vkit/imgui/window_imgui_host.hpp"

namespace vkit::renderer::cmd {

class DrawImGuiCommand : public graphics::Command {
 public:
  DrawImGuiCommand(imgui::WindowImguiHost& imguiHost, std::uint32_t frameIndex)
      : imguiHost_{imguiHost}, frameIndex_{frameIndex} {}

  void record(vk::CommandBuffer cb) const override {
    imguiHost_.render(frameIndex_, cb);
  }

 private:
  imgui::WindowImguiHost& imguiHost_;
  std::uint32_t frameIndex_;
};

};  // namespace vkit::renderer::cmd
