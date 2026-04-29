#include "app.hpp"

#include <imgui_internal.h>

#include "vkit/graphics/device.hpp"
#include "vkit/renderer/command/draw_imgui.hpp"
#include "vkit/renderer/render_pass/swapchain.hpp"
#include "vkit/renderer/render_pass/viewport.hpp"

namespace vkit {

App::App() {
  initWindow();
  initVulkan();
  initImgui();
  initViewports();
}

void App::initWindow() {
  glfwContext_ = std::make_unique<window::Context>();

  const auto config = window::WindowConfiguration{
      .show = true,
      .fullscreen = false,
      .size = {1280, 720},
      .title = "vkit",
  };
  window_ = std::make_unique<window::Window>(config);
}

void App::initVulkan() {
  instance_ = std::make_unique<graphics::Instance>();
  surface_ = std::make_unique<graphics::Surface>(*window_, *instance_);
  gfxDevice_ = std::make_unique<graphics::GfxDevice>(*instance_, *surface_);

  swapchain_ = std::make_unique<graphics::Swapchain>(
      *gfxDevice_, surface_->get(), kMaxFramesInFlight, glm::ivec2{1280, 720});

  renderer_ = std::make_unique<renderer::Renderer>(
      gfxDevice_->get(), gfxDevice_->queues.graphicsPresent,
      gfxDevice_->queueFamilies.graphicsPresent, kMaxFramesInFlight);

  imageAvailableSemaphores_.reserve(kMaxFramesInFlight);
  for (int i = 0; i < kMaxFramesInFlight; ++i) {
    imageAvailableSemaphores_.push_back(
        gfxDevice_->get().createSemaphoreUnique({}));
  }

  waiter_ = graphics::DeviceWaiter{gfxDevice_->get()};
}

void App::initImgui() {
  imguiRenderer_ = std::make_unique<imgui::ImguiRenderer>(
      gfxDevice_->get(), gfxDevice_->allocator, swapchain_->getFormat(),
      vk::SampleCountFlagBits::e1, kMaxFramesInFlight);

  imguiHost_ =
      std::make_unique<imgui::WindowImguiHost>(*imguiRenderer_, "vkit", "");

  imguiHost_->setDockLayoutCallback(
      [](vkit::imgui::WindowImguiHost& host, ImVec2 availableSize) {
        ImGuiID root_id = host.getRootDockId();
        if (ImGui::DockBuilderGetNode(root_id) == nullptr ||
            ImGui::DockBuilderGetNode(root_id)->ChildNodes[0] == nullptr) {
          ImGui::DockBuilderRemoveNode(root_id);
          ImGui::DockBuilderAddNode(root_id, ImGuiDockNodeFlags_DockSpace);
          ImGui::DockBuilderSetNodeSize(root_id, availableSize);

          ImGuiID dock_main_id = root_id;
          ImGuiID dock_left_id = ImGui::DockBuilderSplitNode(
              dock_main_id, ImGuiDir_Left, 0.20F, nullptr, &dock_main_id);
          ImGuiID dock_right_id = ImGui::DockBuilderSplitNode(
              dock_main_id, ImGuiDir_Right, 0.25F, nullptr, &dock_main_id);

          ImGui::DockBuilderDockWindow("Red Viewport", dock_left_id);
          ImGui::DockBuilderDockWindow("Green Viewport", dock_right_id);
          ImGui::DockBuilderDockWindow("Blue Viewport", dock_main_id);

          ImGui::DockBuilderFinish(root_id);
        }
      });

  const auto submit_info = graphics::util::RecordAndSubmitInfo{
      .device = gfxDevice_->get(),
      .queue = gfxDevice_->queues.graphicsPresent,
      .commandPool = gfxDevice_->getGraphicsPresentCommandPool(),
  };

  imguiRenderer_->uploadFont(submit_info);
  windowManager_ = std::make_unique<imgui::ImguiWindowManager>();
}

void App::initViewports() {
  viewportRed_ = std::make_shared<imgui::windows::BufferedViewport>(
      "Red Viewport", gfxDevice_->get(), gfxDevice_->allocator, *imguiRenderer_,
      kMaxFramesInFlight);

  viewportGreen_ = std::make_shared<imgui::windows::BufferedViewport>(
      "Green Viewport", gfxDevice_->get(), gfxDevice_->allocator,
      *imguiRenderer_, kMaxFramesInFlight);

  viewportBlue_ = std::make_shared<imgui::windows::BufferedViewport>(
      "Blue Viewport", gfxDevice_->get(), gfxDevice_->allocator,
      *imguiRenderer_, kMaxFramesInFlight);

  windowManager_->addWindow(viewportRed_);
  windowManager_->addWindow(viewportGreen_);
  windowManager_->addWindow(viewportBlue_);
}

void App::run() { mainLoop(); }

void App::mainLoop() {
  auto last_time = std::chrono::steady_clock::now();
  auto swapchain_needs_recreation = false;

  while (!window_->shouldClose()) {
    window_->pollEvents();

    auto current_time = std::chrono::steady_clock::now();
    float dt = std::chrono::duration<float, std::chrono::seconds::period>(
                   current_time - last_time)
                   .count();
    last_time = current_time;

    for (auto& event : window_->getInputEvents()) {
      imguiHost_->dispatchInputEvent(event);
      if (event.type == window::InputEventType::kWindowResizeEvent) {
        swapchain_needs_recreation = true;
      }
    }

    if (swapchain_needs_recreation) {
      recreateSwapchain();
      swapchain_needs_recreation = false;
      continue;
    }

    renderer_->beginFrame(currentFrame_);

    auto image_index_opt =
        swapchain_->acquireNextImage(*imageAvailableSemaphores_[currentFrame_]);
    if (!image_index_opt) {
      swapchain_needs_recreation = true;
      continue;
    }
    const auto image_index = image_index_opt.value();

    imguiHost_->beginFrame(window_->getWidth(), window_->getHeight(), dt);
    windowManager_->drawWindows(currentFrame_);
    imguiHost_->endFrame();

    auto task = renderer::RenderTask{};

    task.add<renderer::rp::BeginViewportPass>(
            viewportRed_->getViewport(currentFrame_),
            std::array<float, 4>{1.0F, 0.0F, 0.0F, 1.0F})
        .add<renderer::rp::EndViewportPass>(
            viewportRed_->getViewport(currentFrame_));

    task.add<renderer::rp::BeginViewportPass>(
            viewportGreen_->getViewport(currentFrame_),
            std::array<float, 4>{0.0F, 1.0F, 0.0F, 1.0F})
        .add<renderer::rp::EndViewportPass>(
            viewportGreen_->getViewport(currentFrame_));

    task.add<renderer::rp::BeginViewportPass>(
            viewportBlue_->getViewport(currentFrame_),
            std::array<float, 4>{0.0F, 0.0F, 1.0F, 1.0F})
        .add<renderer::rp::EndViewportPass>(
            viewportBlue_->getViewport(currentFrame_));

    task.add<renderer::rp::BeginSwapchainPass>(
            swapchain_->getImage(image_index),
            swapchain_->getImageView(image_index), swapchain_->getExtent(),
            std::array<float, 4>{0.02F, 0.02F, 0.02F, 1.0F})
        .add<renderer::command::DrawImGui>(*imguiHost_, currentFrame_)
        .add<renderer::rp::EndSwapchainPass>(swapchain_->getImage(image_index));

    auto result = renderer_->submit(currentFrame_, task,
                                    *imageAvailableSemaphores_[currentFrame_]);

    const auto presented =
        swapchain_->present(image_index, result.renderFinishedSemaphore);
    if (!presented) {
      swapchain_needs_recreation = true;
    }

    currentFrame_ = (currentFrame_ + 1) % kMaxFramesInFlight;
  }
}

void App::recreateSwapchain() {
  auto width = window_->getWidth();
  auto height = window_->getHeight();

  if (width >= 0 && height >= 0) {
    gfxDevice_->waitIdle();
    swapchain_->recreate(glm::ivec2{width, height});
  }
}

};  // namespace vkit
