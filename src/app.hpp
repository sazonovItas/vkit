#pragma once

#include <memory>
#include <vector>
#include <vk_mem_alloc.hpp>

#include "vkit/graphics/device.hpp"
#include "vkit/graphics/instance.hpp"
#include "vkit/graphics/surface.hpp"
#include "vkit/graphics/swapchain.hpp"
#include "vkit/imgui/imgui_renderer.hpp"
#include "vkit/imgui/imgui_window_manager.hpp"
#include "vkit/imgui/window_imgui_host.hpp"
#include "vkit/imgui/windows/buffered_viewport.hpp"
#include "vkit/renderer/renderer.hpp"

namespace vkit {

class App {
 public:
  App();

  void run();

 private:
  void initWindow();
  void initVulkan();
  void initImgui();
  void initWindows();

  void mainLoop();
  void recreateSwapchain();

  static constexpr int kMaxFramesInFlight = 3;
  std::uint32_t currentFrame_{0};

  std::unique_ptr<window::Context> glfwContext_;
  std::unique_ptr<window::Window> window_;

  std::unique_ptr<graphics::Instance> instance_;
  std::unique_ptr<graphics::Surface> surface_;
  std::unique_ptr<graphics::GfxDevice> gfxDevice_;
  std::unique_ptr<graphics::Swapchain> swapchain_;

  std::unique_ptr<renderer::Renderer> renderer_;

  std::unique_ptr<imgui::ImguiRenderer> imguiRenderer_;
  std::unique_ptr<imgui::WindowImguiHost> imguiHost_;
  std::unique_ptr<imgui::ImguiWindowManager> windowManager_;

  std::vector<vk::UniqueSemaphore> imageAvailableSemaphores_;

  std::shared_ptr<imgui::windows::BufferedViewport> viewportScene_;
  std::shared_ptr<imgui::windows::BufferedViewport> viewportMaterial_;

  graphics::DeviceWaiter waiter_;
};

};  // namespace vkit
