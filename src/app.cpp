#include "app.hpp"

#include <imgui_internal.h>

#include <tuple>

#include "vkit/asset/shaders.hpp"
#include "vkit/asset/util.hpp"
#include "vkit/graphics/device.hpp"
#include "vkit/graphics/pipeline/graphics.hpp"
#include "vkit/graphics/shader_module.hpp"
#include "vkit/renderer/command/draw_imgui.hpp"
#include "vkit/renderer/render_pass/swapchain.hpp"
#include "vkit/renderer/render_pass/viewport.hpp"
#include "vkit/renderer/viewport.hpp"
#include "vulkan/vulkan.hpp"

namespace vkit {

namespace {

class DrawRaySphereCommand final : public graphics::Command {
 public:
  DrawRaySphereCommand(vk::Pipeline pipeline, vk::PipelineLayout layout,
                       vk::DescriptorSet set, glm::mat4 model)
      : pipeline_{pipeline}, layout_{layout}, set_{set}, model_{model} {}

  void record(vk::CommandBuffer cb) const override {
    cb.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline_);
    cb.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, layout_, 0, 1,
                          &set_, 0, nullptr);

    renderer::pl::RaySphereDebugPipelineLayout::PushConstants pcs{.model =
                                                                      model_};
    cb.pushConstants(
        layout_,
        vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
        0, sizeof(renderer::pl::RaySphereDebugPipelineLayout::PushConstants),
        &pcs);

    cb.draw(36, 1, 0, 0);
  }

 private:
  vk::Pipeline pipeline_;
  vk::PipelineLayout layout_;
  vk::DescriptorSet set_;
  glm::mat4 model_;
};

struct alignas(16) CameraUBO {
  glm::mat4 view;
  glm::mat4 proj;
  alignas(16) glm::vec3 position;
};

};  // namespace

App::App() {
  initWindow();
  initVulkan();
  initImgui();
  initViewports();
  initDebugRendering();
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
          ImGuiID dock_right_id = ImGui::DockBuilderSplitNode(
              dock_main_id, ImGuiDir_Right, 0.25F, nullptr, &dock_main_id);

          ImGuiID dock_bottom_left_id = ImGui::DockBuilderSplitNode(
              dock_main_id, ImGuiDir_Down, 0.25F, nullptr, &dock_main_id);

          ImGuiID dock_top_right_id = ImGui::DockBuilderSplitNode(
              dock_right_id, ImGuiDir_Up, 0.30F, nullptr, &dock_right_id);

          ImGui::DockBuilderDockWindow("Scene", dock_main_id);
          ImGui::DockBuilderDockWindow("Graph", dock_bottom_left_id);
          ImGui::DockBuilderDockWindow("Material Preview", dock_top_right_id);
          ImGui::DockBuilderDockWindow("Configuration", dock_right_id);

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
  sceneCamera_ = std::make_shared<scene::Camera>("Scene Camera");
  sceneCamera_->setPosition(glm::vec3(0.0F, 2.0F, 5.0F));

  auto viewport_info = renderer::ViewportInfo{};
  viewport_info.addColorTarget(vk::Format::eR8G8B8A8Unorm,
                               vk::ImageUsageFlagBits::eColorAttachment,
                               vk::SampleCountFlagBits::e8);
  viewport_info.addColorTarget(vk::Format::eR8G8B8A8Unorm,
                               vk::ImageUsageFlagBits::eColorAttachment |
                                   vk::ImageUsageFlagBits::eSampled,
                               vk::SampleCountFlagBits::e1);
  viewport_info.setDepthTarget(vk::Format::eD32Sfloat,
                               vk::ImageUsageFlagBits::eDepthStencilAttachment,
                               vk::SampleCountFlagBits::e8);

  viewportScene_ = std::make_shared<imgui::windows::BufferedViewport>(
      "Scene", gfxDevice_->get(), gfxDevice_->allocator, *imguiRenderer_,
      viewport_info, kMaxFramesInFlight, 1);

  viewportMaterial_ = std::make_shared<imgui::windows::BufferedViewport>(
      "Material Preview", gfxDevice_->get(), gfxDevice_->allocator,
      *imguiRenderer_, viewport_info, kMaxFramesInFlight, 1);

  materialCamera_ = std::make_shared<scene::Camera>("Material Camera");
  materialController_.target = glm::vec3(0.0F, 0.0F, 0.0F);
  materialController_.distance = 2.5F;

  windowManager_->addWindow(viewportScene_);
  windowManager_->addWindow(viewportMaterial_);
}

void App::initDebugRendering() {
  vk::Device device = gfxDevice_->get();

  sceneSetLayout_ = std::make_unique<renderer::dsl::SceneSetLayout>(device);
  raySpherePipelineLayout_ =
      std::make_unique<renderer::pl::RaySphereDebugPipelineLayout>(
          device, std::forward_as_tuple(*sceneSetLayout_));

  auto vert_module = graphics::SpirVShaderModule{
      device, asset::assetPath(asset::kRaySphereVertShaderPath)};
  auto frag_module = graphics::SpirVShaderModule{
      device, asset::assetPath(asset::kRaySphereDebugFragShaderPath)};

  auto builder = graphics::pipeline::GraphicsPipelineBuilder{
      raySpherePipelineLayout_->get()};
  builder
      .addShaderStage(
          vert_module.stageCreateInfo(vk::ShaderStageFlagBits::eVertex))
      .addShaderStage(
          frag_module.stageCreateInfo(vk::ShaderStageFlagBits::eFragment))
      .setVertexInput({}, {})
      .setRenderingFormats({vk::Format::eR8G8B8A8Unorm}, vk::Format::eD32Sfloat)
      .setDepthState(vk::True, vk::True)
      .setCullMode(vk::CullModeFlagBits::eNone)
      .setMultisampling(vk::SampleCountFlagBits::e8, vk::True, 0.5F);

  raySpherePipeline_ = builder.build(device);

  vk::DescriptorPoolSize pool_size{vk::DescriptorType::eUniformBuffer,
                                   kMaxFramesInFlight * 2};
  vk::DescriptorPoolCreateInfo pool_info{};
  pool_info.setFlags(vk::DescriptorPoolCreateFlagBits::eUpdateAfterBind)
      .setMaxSets(kMaxFramesInFlight * 2)
      .setPoolSizes(pool_size);
  descriptorPool_ = device.createDescriptorPoolUnique(pool_info);

  sceneDescriptorSets_.resize(kMaxFramesInFlight);
  materialDescriptorSets_.resize(kMaxFramesInFlight);

  for (int i = 0; i < kMaxFramesInFlight; ++i) {
    sceneCameraBuffers_.push_back(std::make_unique<graphics::DescriptorBuffer>(
        gfxDevice_->allocator, static_cast<dataformat::BufferUsageFlags>(
                                   vk::BufferUsageFlagBits::eUniformBuffer)));

    materialCameraBuffers_.push_back(
        std::make_unique<graphics::DescriptorBuffer>(
            gfxDevice_->allocator,
            static_cast<dataformat::BufferUsageFlags>(
                vk::BufferUsageFlagBits::eUniformBuffer)));

    vk::DescriptorSetAllocateInfo alloc_info{*descriptorPool_, 1,
                                             &sceneSetLayout_->get()};

    sceneDescriptorSets_[i] = device.allocateDescriptorSets(alloc_info)[0];
    materialDescriptorSets_[i] = device.allocateDescriptorSets(alloc_info)[0];

    auto scene_buffer_info = sceneCameraBuffers_[i]->descriptorInfo();
    vk::WriteDescriptorSet scene_write{
        sceneDescriptorSets_[i],
        renderer::dsl::SceneSetLayout::kCameraBinding,
        0,
        1,
        vk::DescriptorType::eUniformBuffer,
        nullptr,
        &scene_buffer_info,
        nullptr};

    auto mat_buffer_info = materialCameraBuffers_[i]->descriptorInfo();
    vk::WriteDescriptorSet mat_write{
        materialDescriptorSets_[i],
        renderer::dsl::SceneSetLayout::kCameraBinding,
        0,
        1,
        vk::DescriptorType::eUniformBuffer,
        nullptr,
        &mat_buffer_info,
        nullptr};

    device.updateDescriptorSets({scene_write, mat_write}, nullptr);
  }
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

    auto& io = ImGui::GetIO();

    if (viewportScene_->isHovered()) {
      if (ImGui::IsMouseDown(ImGuiMouseButton_Left) ||
          ImGui::IsMouseDown(ImGuiMouseButton_Right)) {
        sceneController_.processMouseMovement(io.MouseDelta.x, io.MouseDelta.y);
      }
      if (io.MouseWheel != 0.0F) {
        sceneController_.processScroll(io.MouseWheel);
      }
    }

    if (viewportMaterial_->isHovered()) {
      if (ImGui::IsMouseDown(ImGuiMouseButton_Left) ||
          ImGui::IsMouseDown(ImGuiMouseButton_Right)) {
        materialController_.processMouseMovement(io.MouseDelta.x,
                                                 io.MouseDelta.y);
      }
      if (io.MouseWheel != 0.0F) {
        materialController_.processScroll(io.MouseWheel);
      }
    }

    if (viewportScene_->getWidth() > 0 && viewportScene_->getHeight() > 0) {
      sceneCamera_->setAspectRatio(
          static_cast<float>(viewportScene_->getWidth()) /
          static_cast<float>(viewportScene_->getHeight()));
    }
    if (viewportMaterial_->getWidth() > 0 &&
        viewportMaterial_->getHeight() > 0) {
      materialCamera_->setAspectRatio(
          static_cast<float>(viewportMaterial_->getWidth()) /
          static_cast<float>(viewportMaterial_->getHeight()));
    }

    sceneController_.update(*sceneCamera_);
    materialController_.update(*materialCamera_);

    CameraUBO scene_ubo{
        .view = sceneCamera_->getViewMatrix(),
        .proj = sceneCamera_->getProjectionMatrix(),
        .position = sceneCamera_->getPosition(),
    };
    sceneCameraBuffers_[currentFrame_]->writeAt(
        std::as_bytes(std::span{&scene_ubo, 1}));

    CameraUBO mat_ubo{
        .view = materialCamera_->getViewMatrix(),
        .proj = materialCamera_->getProjectionMatrix(),
        .position = materialCamera_->getPosition(),
    };
    materialCameraBuffers_[currentFrame_]->writeAt(
        std::as_bytes(std::span{&mat_ubo, 1}));

    ImGui::Begin("Graph");
    ImGui::Text("Node Graph");
    ImGui::End();
    ImGui::Begin("Configuration");
    ImGui::Text("Inspector");
    ImGui::End();

    windowManager_->drawWindows(currentFrame_);
    imguiHost_->endFrame();

    auto task = renderer::RenderTask{};

    task.add<renderer::rp::BeginViewportPass>(
            viewportScene_->getViewport(currentFrame_), 0, 1)
        .add<DrawRaySphereCommand>(
            *raySpherePipeline_, raySpherePipelineLayout_->get(),
            sceneDescriptorSets_[currentFrame_], glm::mat4(1.0F))
        .add<renderer::rp::EndViewportPass>(
            viewportScene_->getViewport(currentFrame_), 1);

    task.add<renderer::rp::BeginViewportPass>(
            viewportMaterial_->getViewport(currentFrame_), 0, 1)
        .add<DrawRaySphereCommand>(
            *raySpherePipeline_, raySpherePipelineLayout_->get(),
            materialDescriptorSets_[currentFrame_], glm::mat4(1.0F))
        .add<renderer::rp::EndViewportPass>(
            viewportMaterial_->getViewport(currentFrame_), 1);

    task.add<renderer::rp::BeginSwapchainPass>(
            swapchain_->getImage(image_index),
            swapchain_->getImageView(image_index), swapchain_->getExtent())
        .add<renderer::cmd::DrawImGuiCommand>(*imguiHost_, currentFrame_)
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

  if (width > 0 && height > 0) {
    gfxDevice_->waitIdle();
    swapchain_->recreate(glm::ivec2{width, height});
  }
}

};  // namespace vkit
