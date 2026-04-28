#pragma once

#include <array>
#include <memory>
#include <vector>
#include <vk_mem_alloc.hpp>
#include <vulkan/vulkan.hpp>

#include "vkit/graphics/device.hpp"
#include "vkit/graphics/image.hpp"
#include "vkit/graphics/instance.hpp"
#include "vkit/graphics/surface.hpp"
#include "vkit/graphics/swapchain.hpp"
#include "vkit/imgui/imgui_renderer.hpp"
#include "vkit/imgui/imgui_window_manager.hpp"
#include "vkit/imgui/window_imgui_host.hpp"
#include "vkit/imgui/windows/viewport.hpp"
#include "vkit/window/window.hpp"

namespace vkit {

class App {
 public:
  App();
  ~App();

  void run();

 private:
  void initWindow();
  void initVulkan();
  void initImgui();
  void createDummyTextures();

  void mainLoop();
  void recreateSwapchain();

  static constexpr int kMaxFramesInFlight = 3;
  std::uint32_t currentFrame_{0};

  std::unique_ptr<window::Context> glfwContext_;
  std::unique_ptr<window::Window> window_;

  std::unique_ptr<graphics::Instance> instance_;
  std::unique_ptr<graphics::Surface> surface_;
  std::shared_ptr<graphics::GfxDevice> gfxDevice_;
  std::unique_ptr<graphics::Swapchain> swapchain_;

  std::unique_ptr<imgui::ImguiRenderer> imguiRenderer_;
  std::unique_ptr<imgui::WindowImguiHost> imguiHost_;
  std::unique_ptr<imgui::ImguiWindowManager> windowManager_;

  std::shared_ptr<imgui::windows::ViewportWindow> viewportRed_;
  std::shared_ptr<imgui::windows::ViewportWindow> viewportGreen_;
  std::shared_ptr<imgui::windows::ViewportWindow> viewportBlue_;

  vk::UniqueCommandPool commandPool_;
  std::vector<vk::UniqueCommandBuffer> commandBuffers_;
  std::vector<vk::UniqueSemaphore> imageAvailableSemaphores_;
  std::vector<vk::UniqueSemaphore> renderFinishedSemaphores_;
  std::vector<vk::UniqueFence> inFlightFences_;

  struct DummyTexture {
    graphics::AllocatedImage image;
    vk::UniqueImageView view;
    ImTextureID imguiId{0};
  };

  std::vector<DummyTexture> dummyTextures_;
  vk::UniqueSampler defaultSampler_;

  auto create1x1Texture(std::array<uint8_t, 4> color) -> DummyTexture;
};

}  // namespace vkit
