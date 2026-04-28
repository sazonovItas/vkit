#include "app.hpp"

#include <imgui.h>
#include <imgui_internal.h>

#include "vkit/graphics/util.hpp"

namespace vkit {

App::App() {
  initWindow();
  initVulkan();
  initImgui();
  createDummyTextures();
}

App::~App() {
  if (gfxDevice_) {
    gfxDevice_->waitIdle();
  }

  for (auto& tex : dummyTextures_) {
    if (tex.imguiId) {
      imguiRenderer_->unregisterTexture(tex.imguiId);
    }
  }
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
  gfxDevice_ = std::make_shared<graphics::GfxDevice>(*instance_, *surface_);

  swapchain_ = std::make_unique<graphics::Swapchain>(
      *gfxDevice_, surface_->get(), 3, glm::ivec2{1280, 720});

  auto const device = gfxDevice_->get();

  commandPool_ =
      gfxDevice_->createCommandPool(gfxDevice_->queueFamilies.graphicsPresent);

  commandBuffers_.reserve(kMaxFramesInFlight);
  imageAvailableSemaphores_.reserve(kMaxFramesInFlight);
  renderFinishedSemaphores_.reserve(kMaxFramesInFlight);
  inFlightFences_.reserve(kMaxFramesInFlight);

  auto alloc_info = vk::CommandBufferAllocateInfo{}
                        .setCommandPool(*commandPool_)
                        .setLevel(vk::CommandBufferLevel::ePrimary)
                        .setCommandBufferCount(kMaxFramesInFlight);

  commandBuffers_ = device.allocateCommandBuffersUnique(alloc_info);

  for (int i = 0; i < kMaxFramesInFlight; ++i) {
    imageAvailableSemaphores_.push_back(device.createSemaphoreUnique({}));
    renderFinishedSemaphores_.push_back(device.createSemaphoreUnique({}));
    inFlightFences_.push_back(
        device.createFenceUnique({vk::FenceCreateFlagBits::eSignaled}));
  }
}

void App::initImgui() {
  imguiRenderer_ = std::make_unique<imgui::ImguiRenderer>(
      gfxDevice_->get(), gfxDevice_->allocator, swapchain_->getFormat(),
      vk::SampleCountFlagBits::e1, kMaxFramesInFlight);

  imguiHost_ =
      std::make_unique<imgui::WindowImguiHost>(*imguiRenderer_, "vkit", "");
  imguiHost_->setStyle();

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

  std::optional<graphics::MappedBuffer> font_staging;
  auto const submit_info = graphics::util::RecordAndSubmitInfo{
      .device = gfxDevice_->get(),
      .queue = gfxDevice_->queues.graphicsPresent,
      .commandPool = gfxDevice_->getGraphicsPresentCommandPool()};

  graphics::util::recordAndSubmit(submit_info, [&](vk::CommandBuffer cb) {
    font_staging = imguiRenderer_->uploadFont(cb);
  });

  windowManager_ = std::make_unique<imgui::ImguiWindowManager>();

  auto dummy_resize = [](std::uint32_t w, std::uint32_t h) {};

  viewportRed_ = std::make_shared<imgui::windows::ViewportWindow>(
      "Red Viewport", dummy_resize);
  viewportGreen_ = std::make_shared<imgui::windows::ViewportWindow>(
      "Green Viewport", dummy_resize);
  viewportBlue_ = std::make_shared<imgui::windows::ViewportWindow>(
      "Blue Viewport", dummy_resize);

  windowManager_->addWindow(viewportRed_);
  windowManager_->addWindow(viewportGreen_);
  windowManager_->addWindow(viewportBlue_);
}

void App::createDummyTextures() {
  auto sampler_ci = vk::SamplerCreateInfo{}
                        .setMagFilter(vk::Filter::eNearest)
                        .setMinFilter(vk::Filter::eNearest);
  defaultSampler_ = gfxDevice_->get().createSamplerUnique(sampler_ci);

  dummyTextures_.push_back(create1x1Texture({255, 0, 0, 255}));
  dummyTextures_.push_back(create1x1Texture({0, 255, 0, 255}));
  dummyTextures_.push_back(create1x1Texture({0, 0, 255, 255}));

  viewportRed_->setCurrentTexture(dummyTextures_[0].imguiId);
  viewportGreen_->setCurrentTexture(dummyTextures_[1].imguiId);
  viewportBlue_->setCurrentTexture(dummyTextures_[2].imguiId);
}

void App::run() { mainLoop(); }

void App::mainLoop() {
  const auto device = gfxDevice_->get();
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
      bool handled = imguiHost_->dispatchInputEvent(event);

      if (event.type == window::InputEventType::kWindowResizeEvent) {
        swapchain_needs_recreation = true;
      }
    }

    if (swapchain_needs_recreation) {
      recreateSwapchain();
      swapchain_needs_recreation = false;
      continue;
    }

    std::ignore = device.waitForFences(*inFlightFences_[currentFrame_],
                                       vk::True, UINT64_MAX);

    auto image_index_opt =
        swapchain_->acquireNextImage(*imageAvailableSemaphores_[currentFrame_]);

    if (!image_index_opt) {
      swapchain_needs_recreation = true;
      continue;
    }

    device.resetFences(*inFlightFences_[currentFrame_]);
    const auto image_index = image_index_opt.value();

    imguiHost_->beginFrame(window_->getWidth(), window_->getHeight(), dt);
    windowManager_->drawWindows();
    imguiHost_->endFrame();

    const auto& cb = *commandBuffers_[currentFrame_];

    cb.reset();
    auto begin_info = vk::CommandBufferBeginInfo{}.setFlags(
        vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
    cb.begin(begin_info);

    auto barrier = swapchain_->imageBaseBarrier(image_index);
    barrier.setOldLayout(vk::ImageLayout::eUndefined)
        .setNewLayout(vk::ImageLayout::eColorAttachmentOptimal)
        .setSrcAccessMask(vk::AccessFlagBits2::eNone)
        .setDstAccessMask(vk::AccessFlagBits2::eColorAttachmentWrite)
        .setSrcStageMask(vk::PipelineStageFlagBits2::eTopOfPipe)
        .setDstStageMask(vk::PipelineStageFlagBits2::eColorAttachmentOutput);
    cb.pipelineBarrier2(vk::DependencyInfo{}.setImageMemoryBarriers(barrier));

    auto color_attachment =
        vk::RenderingAttachmentInfo{}
            .setImageView(swapchain_->getImageView(image_index))
            .setImageLayout(vk::ImageLayout::eColorAttachmentOptimal)
            .setLoadOp(vk::AttachmentLoadOp::eClear)
            .setStoreOp(vk::AttachmentStoreOp::eStore)
            .setClearValue(vk::ClearColorValue{
                std::array<float, 4>{0.02F, 0.02F, 0.02F, 1.0F}});

    auto render_info =
        vk::RenderingInfo{}
            .setRenderArea(vk::Rect2D{{0, 0}, swapchain_->getExtent()})
            .setLayerCount(1)
            .setColorAttachments(color_attachment);

    cb.beginRendering(render_info);

    imguiHost_->render(cb, currentFrame_);

    cb.endRendering();

    barrier.setOldLayout(vk::ImageLayout::eColorAttachmentOptimal)
        .setNewLayout(vk::ImageLayout::ePresentSrcKHR)
        .setSrcAccessMask(vk::AccessFlagBits2::eColorAttachmentWrite)
        .setDstAccessMask(vk::AccessFlagBits2::eNone)
        .setSrcStageMask(vk::PipelineStageFlagBits2::eColorAttachmentOutput)
        .setDstStageMask(vk::PipelineStageFlagBits2::eBottomOfPipe);
    cb.pipelineBarrier2(vk::DependencyInfo{}.setImageMemoryBarriers(barrier));

    cb.end();

    vk::PipelineStageFlags wait_stages =
        vk::PipelineStageFlagBits::eColorAttachmentOutput;

    auto submit_info =
        vk::SubmitInfo{}
            .setWaitSemaphores(*imageAvailableSemaphores_[currentFrame_])
            .setWaitDstStageMask(wait_stages)
            .setCommandBuffers(cb)
            .setSignalSemaphores(*renderFinishedSemaphores_[currentFrame_]);

    gfxDevice_->queues.graphicsPresent.submit(submit_info,
                                              *inFlightFences_[currentFrame_]);

    const auto presented = swapchain_->present(
        image_index, *renderFinishedSemaphores_[currentFrame_]);
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

auto App::create1x1Texture(std::array<uint8_t, 4> color) -> DummyTexture {
  auto image_ci = vk::ImageCreateInfo{}
                      .setImageType(vk::ImageType::e2D)
                      .setFormat(vk::Format::eR8G8B8A8Unorm)
                      .setExtent({1, 1, 1})
                      .setMipLevels(1)
                      .setArrayLayers(1)
                      .setSamples(vk::SampleCountFlagBits::e1)
                      .setTiling(vk::ImageTiling::eOptimal)
                      .setUsage(vk::ImageUsageFlagBits::eSampled |
                                vk::ImageUsageFlagBits::eTransferDst);

  auto tex = DummyTexture{
      .image =
          graphics::AllocatedImage{
              gfxDevice_->allocator,
              image_ci,
              graphics::allocation::kDeviceLocal,
          },
  };

  auto view_ci =
      vk::ImageViewCreateInfo{}
          .setImage(static_cast<vk::Image>(tex.image))
          .setViewType(vk::ImageViewType::e2D)
          .setFormat(vk::Format::eR8G8B8A8Unorm)
          .setSubresourceRange({vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1});
  tex.view = gfxDevice_->get().createImageViewUnique(view_ci);

  auto staging_ci = vk::BufferCreateInfo{}.setSize(4).setUsage(
      vk::BufferUsageFlagBits::eTransferSrc);
  auto staging = graphics::MappedBuffer{gfxDevice_->allocator, staging_ci,
                                        graphics::allocation::kHostWrite};
  std::memcpy(staging.data, color.data(), 4);

  auto const submit_info = graphics::util::RecordAndSubmitInfo{
      .device = gfxDevice_->get(),
      .queue = gfxDevice_->queues.graphicsPresent,
      .commandPool = gfxDevice_->getGraphicsPresentCommandPool()};

  graphics::util::recordAndSubmit(submit_info, [&](vk::CommandBuffer cb) {
    auto barrier =
        vk::ImageMemoryBarrier2{}
            .setImage(static_cast<vk::Image>(tex.image))
            .setOldLayout(vk::ImageLayout::eUndefined)
            .setNewLayout(vk::ImageLayout::eTransferDstOptimal)
            .setDstStageMask(vk::PipelineStageFlagBits2::eTransfer)
            .setDstAccessMask(vk::AccessFlagBits2::eTransferWrite)
            .setSubresourceRange({vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1});
    cb.pipelineBarrier2(vk::DependencyInfo{}.setImageMemoryBarriers(barrier));

    auto region = vk::BufferImageCopy{
        0,         0,        0, {vk::ImageAspectFlagBits::eColor, 0, 0, 1},
        {0, 0, 0}, {1, 1, 1}};
    cb.copyBufferToImage(static_cast<vk::Buffer>(staging),
                         static_cast<vk::Image>(tex.image),
                         vk::ImageLayout::eTransferDstOptimal, region);

    barrier.setOldLayout(vk::ImageLayout::eTransferDstOptimal)
        .setNewLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
        .setSrcStageMask(vk::PipelineStageFlagBits2::eTransfer)
        .setSrcAccessMask(vk::AccessFlagBits2::eTransferWrite)
        .setDstStageMask(vk::PipelineStageFlagBits2::eFragmentShader)
        .setDstAccessMask(vk::AccessFlagBits2::eShaderRead);
    cb.pipelineBarrier2(vk::DependencyInfo{}.setImageMemoryBarriers(barrier));
  });

  tex.imguiId = imguiRenderer_->registerTexture(*tex.view, *defaultSampler_);
  return tex;
}

}  // namespace vkit
