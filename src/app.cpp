#include "app.hpp"

#include <imgui_internal.h>

#include <algorithm>
#include <filesystem>
#include <print>
#include <tuple>

#include "vkit/asset/gltf/importer.hpp"
#include "vkit/asset/shaders.hpp"
#include "vkit/asset/util.hpp"
#include "vkit/graphics/device.hpp"
#include "vkit/graphics/pipeline/graphics.hpp"
#include "vkit/graphics/shader_module.hpp"
#include "vkit/renderer/command/draw_imgui.hpp"
#include "vkit/renderer/render_pass/swapchain.hpp"
#include "vkit/renderer/render_pass/viewport.hpp"
#include "vkit/renderer/viewport.hpp"

namespace vkit {
using glm::quat;

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

class DrawAssetCommand final : public graphics::Command {
 public:
  DrawAssetCommand(vk::Pipeline pipeline, vk::PipelineLayout layout,
                   vk::DescriptorSet sceneSet, vk::DescriptorSet primSet,
                   const asset::Asset& asset)
      : pipeline_{pipeline},
        layout_{layout},
        sceneSet_{sceneSet},
        primSet_{primSet},
        asset_{asset} {}

  void record(vk::CommandBuffer cb) const override {
    cb.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline_);

    std::array<vk::DescriptorSet, 2> sets = {sceneSet_, primSet_};
    cb.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, layout_, 0, sets,
                          nullptr);

    if (asset_.scenes.empty()) return;
    auto active_scene = asset_.getActiveScene();
    if (!active_scene) return;

    for (std::uint32_t root_id : active_scene->rootNodes) {
      if (auto node = asset_.nodes.get(root_id)) {
        drawNode(cb, node.get());
      }
    }
  }

 private:
  void drawNode(vk::CommandBuffer cb, const scene::Node* node) const {
    if (node->mesh) {
      for (const auto& prim : node->mesh->getPrimitives()) {
        if (!prim->attrs.index.isValid() || !prim->getStorageId().has_value())
          continue;

        cb.bindIndexBuffer(prim->getIndexBuffer()->buffer,
                           prim->getIndexBufferOffset(), prim->getIndexType());

        renderer::pl::PrimitiveMaterialPipelineLayout::PushConstants pcs{
            .model = node->getGlobalTransform().getMatrix(),
            .primIndex = prim->getStorageId().value(),
            .skinOffset = asset_.skins.getOffsetForNode(node),
            .enableSkinning = (node->skin != nullptr) ? 1U : 0U,
        };

        cb.pushConstants(layout_,
                         vk::ShaderStageFlagBits::eVertex |
                             vk::ShaderStageFlagBits::eFragment,
                         0, sizeof(pcs), &pcs);

        cb.drawIndexed(prim->attrs.index.info.count, 1, 0, 0, 0);
      }
    }

    for (const auto& child : node->getChildren()) {
      drawNode(cb, child.get());
    }
  }

  vk::Pipeline pipeline_;
  vk::PipelineLayout layout_;
  vk::DescriptorSet sceneSet_;
  vk::DescriptorSet primSet_;
  const asset::Asset& asset_;
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

App::~App() {
  if (gfxDevice_) {
    gfxDevice_->waitIdle();
  }

  for (auto& frame : frames_) {
    if (frame.sceneTextureId) {
      imguiRenderer_->unregisterTexture(frame.sceneTextureId);
    }
    if (frame.materialTextureId) {
      imguiRenderer_->unregisterTexture(frame.materialTextureId);
    }
  }
}

void App::initWindow() {
  glfwContext_ = std::make_unique<window::Context>();
  const auto config = window::WindowConfiguration{
      .show = true, .fullscreen = false, .size = {1280, 720}, .title = "vkit"};
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

  materialCamera_ = std::make_shared<scene::Camera>("Material Camera");
  materialController_.target = glm::vec3(0.0F, 0.0F, 0.0F);
  materialController_.distance = 2.5F;

  sceneViewer_ = std::make_shared<imgui::windows::Viewer>("Scene");
  sceneViewer_->setOnResize([this](std::uint32_t width, std::uint32_t height) {
    if (width > 0 && height > 0) {
      sceneCamera_->setAspectRatio(static_cast<float>(width) /
                                   static_cast<float>(height));
    }
  });
  sceneViewer_->setOnManipulate([this](imgui::windows::Viewer& viewer) {
    auto& io = ImGui::GetIO();
    if (viewer.isHovered() && viewer.isFocused()) {
      if (ImGui::IsMouseDown(ImGuiMouseButton_Left) ||
          ImGui::IsMouseDown(ImGuiMouseButton_Right)) {
        sceneController_.processMouseMovement(io.MouseDelta.x, io.MouseDelta.y);
      }
      if (io.MouseWheel != 0.0F) {
        sceneController_.processScroll(io.MouseWheel);
      }
    }
  });

  materialViewer_ =
      std::make_shared<imgui::windows::Viewer>("Material Preview");
  materialViewer_->setOnResize(
      [this](std::uint32_t width, std::uint32_t height) {
        if (width > 0 && height > 0) {
          materialCamera_->setAspectRatio(static_cast<float>(width) /
                                          static_cast<float>(height));
        }
      });
  materialViewer_->setOnManipulate([this](imgui::windows::Viewer& viewer) {
    auto& io = ImGui::GetIO();
    if (viewer.isHovered() && viewer.isFocused()) {
      if (ImGui::IsMouseDown(ImGuiMouseButton_Left) ||
          ImGui::IsMouseDown(ImGuiMouseButton_Right)) {
        materialController_.processMouseMovement(io.MouseDelta.x,
                                                 io.MouseDelta.y);
      }
      if (io.MouseWheel != 0.0F) {
        materialController_.processScroll(io.MouseWheel);
      }
    }
  });

  windowManager_->addWindow(sceneViewer_);
  windowManager_->addWindow(materialViewer_);

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

  for (auto& frame : frames_) {
    frame.sceneViewport = renderer::Viewport(viewport_info);
    frame.materialViewport = renderer::Viewport(viewport_info);
  }
}

void App::initDebugRendering() {
  vk::Device device = gfxDevice_->get();

  std::filesystem::path asset_path = "assets/models/exosuit/scene.gltf";

  asset::gltf::Importer importer(*gfxDevice_);
  if (!std::filesystem::exists(asset_path)) {
    throw std::runtime_error{"Asset file isn't existing"};
  }

  loadedAsset_ = importer.load(asset_path);
  loadedAsset_->update();

  sceneSetLayout_ = std::make_unique<renderer::dsl::SceneSetLayout>(device);
  primitiveSetLayout_ =
      std::make_unique<renderer::dsl::PrimitiveSetLayout>(device);

  raySpherePipelineLayout_ =
      std::make_unique<renderer::pl::RaySphereDebugPipelineLayout>(
          device, std::forward_as_tuple(*sceneSetLayout_));
  primitivePipelineLayout_ =
      std::make_unique<renderer::pl::PrimitiveMaterialPipelineLayout>(
          device,
          std::forward_as_tuple(*sceneSetLayout_, *primitiveSetLayout_));

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

  auto prim_vert = graphics::SpirVShaderModule{
      device, asset::assetPath(asset::kPrimitiveDebugVertShaderPath)};
  auto prim_frag = graphics::SpirVShaderModule{
      device, asset::assetPath(asset::kPrimitiveDebugFragShaderPath)};

  auto prim_builder = graphics::pipeline::GraphicsPipelineBuilder{
      primitivePipelineLayout_->get()};
  prim_builder
      .addShaderStage(
          prim_vert.stageCreateInfo(vk::ShaderStageFlagBits::eVertex))
      .addShaderStage(
          prim_frag.stageCreateInfo(vk::ShaderStageFlagBits::eFragment))
      .setVertexInput({}, {})
      .setRenderingFormats({vk::Format::eR8G8B8A8Unorm}, vk::Format::eD32Sfloat)
      .setDepthState(vk::True, vk::True)
      .setCullMode(vk::CullModeFlagBits::eBack)
      .setMultisampling(vk::SampleCountFlagBits::e8, vk::True, 0.5F);

  primitivePipeline_ = prim_builder.build(device);

  vk::DescriptorPoolSize pool_sizes[] = {
      {vk::DescriptorType::eUniformBuffer, kMaxFramesInFlight * 2},
      {vk::DescriptorType::eStorageBuffer, kMaxFramesInFlight * 2}};

  vk::DescriptorPoolCreateInfo pool_info{};
  pool_info.setFlags(vk::DescriptorPoolCreateFlagBits::eUpdateAfterBind)
      .setMaxSets(kMaxFramesInFlight * 4)
      .setPoolSizes(pool_sizes);
  descriptorPool_ = device.createDescriptorPoolUnique(pool_info);

  for (auto& frame : frames_) {
    frame.sceneCameraBuffer = std::make_unique<graphics::DescriptorBuffer>(
        gfxDevice_->allocator, static_cast<dataformat::BufferUsageFlags>(
                                   vk::BufferUsageFlagBits::eUniformBuffer));

    frame.materialCameraBuffer = std::make_unique<graphics::DescriptorBuffer>(
        gfxDevice_->allocator, static_cast<dataformat::BufferUsageFlags>(
                                   vk::BufferUsageFlagBits::eUniformBuffer));

    frame.primitiveSSBO = std::make_unique<graphics::DescriptorBuffer>(
        gfxDevice_->allocator,
        static_cast<dataformat::BufferUsageFlags>(
            vk::BufferUsageFlagBits::eStorageBuffer),
        sizeof(primitive::PrimitiveData) *
            std::max<std::size_t>(1000,
                                  loadedAsset_->primitives.getItems().size()));

    frame.jointSSBO = std::make_unique<graphics::DescriptorBuffer>(
        gfxDevice_->allocator,
        static_cast<dataformat::BufferUsageFlags>(
            vk::BufferUsageFlagBits::eStorageBuffer),
        sizeof(glm::mat4) * std::max<std::size_t>(1000, 1000));

    vk::DescriptorSetAllocateInfo cam_alloc_info{*descriptorPool_, 1,
                                                 &sceneSetLayout_->get()};
    frame.sceneDescriptorSet = device.allocateDescriptorSets(cam_alloc_info)[0];
    frame.materialDescriptorSet =
        device.allocateDescriptorSets(cam_alloc_info)[0];

    auto scene_buffer_info = frame.sceneCameraBuffer->descriptorInfo();
    auto mat_buffer_info = frame.materialCameraBuffer->descriptorInfo();

    vk::WriteDescriptorSet scene_write{
        frame.sceneDescriptorSet,
        renderer::dsl::SceneSetLayout::kCameraBinding,
        0,
        1,
        vk::DescriptorType::eUniformBuffer,
        nullptr,
        &scene_buffer_info,
        nullptr};

    vk::WriteDescriptorSet mat_write{
        frame.materialDescriptorSet,
        renderer::dsl::SceneSetLayout::kCameraBinding,
        0,
        1,
        vk::DescriptorType::eUniformBuffer,
        nullptr,
        &mat_buffer_info,
        nullptr};

    vk::DescriptorSetAllocateInfo prim_alloc_info{*descriptorPool_, 1,
                                                  &primitiveSetLayout_->get()};
    frame.primitiveDescriptorSet =
        device.allocateDescriptorSets(prim_alloc_info)[0];

    auto prim_info = frame.primitiveSSBO->descriptorInfo();
    auto joint_info = frame.jointSSBO->descriptorInfo();

    vk::WriteDescriptorSet prim_write{
        frame.primitiveDescriptorSet,
        renderer::dsl::PrimitiveSetLayout::kPrimitiveBinding,
        0,
        1,
        vk::DescriptorType::eStorageBuffer,
        nullptr,
        &prim_info,
        nullptr};

    vk::WriteDescriptorSet joint_write{
        frame.primitiveDescriptorSet,
        renderer::dsl::PrimitiveSetLayout::kJointBinding,
        0,
        1,
        vk::DescriptorType::eStorageBuffer,
        nullptr,
        &joint_info,
        nullptr};

    device.updateDescriptorSets(
        {scene_write, mat_write, prim_write, joint_write}, nullptr);
  }
}

void App::ensureViewportSize(renderer::Viewport& viewport,
                             ImTextureID& textureId, std::uint32_t width,
                             std::uint32_t height,
                             std::uint32_t displayTargetIndex) {
  const auto target_width = std::max(1U, width);
  const auto target_height = std::max(1U, height);

  viewport.ensureSize(gfxDevice_->get(), gfxDevice_->allocator, target_width,
                      target_height);

  if (viewport.colorTargets.size() > displayTargetIndex &&
      viewport.colorTargets[displayTargetIndex].view) {
    textureId = imguiRenderer_->updateOrRegisterTexture(
        textureId, *viewport.colorTargets[displayTargetIndex].view);
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

    auto& frame = frames_[currentFrame_];

    ensureViewportSize(frame.sceneViewport, frame.sceneTextureId,
                       sceneViewer_->getWidth(), sceneViewer_->getHeight(), 1);
    sceneViewer_->setCurrentTexture(frame.sceneTextureId);

    ensureViewportSize(frame.materialViewport, frame.materialTextureId,
                       materialViewer_->getWidth(),
                       materialViewer_->getHeight(), 1);
    materialViewer_->setCurrentTexture(frame.materialTextureId);

    sceneController_.update(*sceneCamera_);
    materialController_.update(*materialCamera_);

    // static float time = 0.0F;
    // time += dt;

    // for (const auto& skin : loadedAsset_->skins.getItems()) {
    //   if (!skin) continue;

    //   std::size_t joints_to_wiggle =
    //       std::min<std::size_t>(3, skin->joints.size());

    //   for (std::size_t i = 0; i < joints_to_wiggle; ++i) {
    //     if (auto joint = skin->joints[i]) {
    //       auto local = joint->getLocalTransform();

    //       float angle = std::sin(time * 2.0F) * 0.5F;
    //       glm::quat spin = glm::angleAxis(angle, glm::vec3(0.0F, 1.0F,
    //       0.0F));

    //       local.setRotation(spin);
    //       joint->setLocalTransform(local);
    //     }
    //   }
    // }

    loadedAsset_->update();

    auto prim_data = loadedAsset_->primitives.getData();
    if (!prim_data.empty()) {
      frame.primitiveSSBO->writeAt(std::as_bytes(prim_data));
    }

    auto joint_data = loadedAsset_->skins.getData();
    if (!joint_data.empty()) {
      std::vector<glm::mat4> identity_joints(joint_data.size(),
                                             glm::mat4(1.0F));
      frame.jointSSBO->writeAt(std::as_bytes(std::span{identity_joints}));
      // frame.jointSSBO->writeAt(std::as_bytes(joint_data));
    }

    CameraUBO scene_ubo{
        .view = sceneCamera_->getViewMatrix(),
        .proj = sceneCamera_->getProjectionMatrix(),
        .position = sceneCamera_->getPosition(),
    };
    frame.sceneCameraBuffer->writeAt(std::as_bytes(std::span{&scene_ubo, 1}));

    CameraUBO mat_ubo{
        .view = materialCamera_->getViewMatrix(),
        .proj = materialCamera_->getProjectionMatrix(),
        .position = materialCamera_->getPosition(),
    };
    frame.materialCameraBuffer->writeAt(std::as_bytes(std::span{&mat_ubo, 1}));

    imguiHost_->beginFrame(window_->getWidth(), window_->getHeight(), dt);

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
            frame.sceneViewport, 0, 1,
            std::array<float, 4>{0.1F, 0.1F, 0.1F, 1.0F})
        .add<DrawAssetCommand>(*primitivePipeline_,
                               primitivePipelineLayout_->get(),
                               frame.sceneDescriptorSet,
                               frame.primitiveDescriptorSet, *loadedAsset_)
        .add<renderer::rp::EndViewportPass>(frame.sceneViewport, 1);

    task.add<renderer::rp::BeginViewportPass>(
            frame.materialViewport, 0, 1,
            std::array<float, 4>{0.15F, 0.15F, 0.15F, 1.0F})
        .add<DrawRaySphereCommand>(*raySpherePipeline_,
                                   raySpherePipelineLayout_->get(),
                                   frame.materialDescriptorSet, glm::mat4(1.0F))
        .add<renderer::rp::EndViewportPass>(frame.materialViewport, 1);

    task.add<renderer::rp::BeginSwapchainPass>(
            swapchain_->getImage(image_index),
            swapchain_->getImageView(image_index), swapchain_->getExtent(),
            std::array<float, 4>{0.02F, 0.02F, 0.02F, 1.0F})
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
