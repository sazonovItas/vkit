#include "app.hpp"

#include <imgui.h>
#include <imgui_internal.h>

#include <algorithm>
#include <filesystem>

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <memory>
#include <span>

#include "vkit/asset/asset_manager.hpp"
#include "vkit/asset/gltf_storage.hpp"
#include "vkit/asset/util.hpp"
#include "vkit/controller/environment.hpp"
#include "vkit/core/events/fractal.hpp"
#include "vkit/core/events/noise.hpp"
#include "vkit/core/events/operators.hpp"
#include "vkit/core/events/pattern.hpp"
#include "vkit/core/shaders/shaders.hpp"
#include "vkit/graphics/bindless_texture_manager.hpp"
#include "vkit/graphics/device.hpp"
#include "vkit/imgui/windows/ge/node/fractal_generator_ui.hpp"
#include "vkit/imgui/windows/ge/node/heightmap_ui.hpp"
#include "vkit/imgui/windows/ge/node/mix_ui.hpp"
#include "vkit/imgui/windows/ge/node/noise_generator_ui.hpp"
#include "vkit/imgui/windows/ge/node/normalmap_ui.hpp"
#include "vkit/imgui/windows/ge/node/pattern_generator_ui.hpp"
#include "vkit/imgui/windows/ge/node/principled_bsdf_ui.hpp"
#include "vkit/imgui/windows/ge/node/slot_output_ui.hpp"
#include "vkit/imgui/windows/ge/node/sobel_ui.hpp"
#include "vkit/imgui/windows/ge/node/texture_load_ui.hpp"
#include "vkit/imgui/windows/ge/node/tint_ui.hpp"
#include "vkit/renderer/command/draw_imgui.hpp"
#include "vkit/renderer/command/draw_light_gizmos.hpp"
#include "vkit/renderer/command/draw_scene.hpp"
#include "vkit/renderer/command/draw_shadow.hpp"
#include "vkit/renderer/render_pass/shadow.hpp"
#include "vkit/renderer/render_pass/swapchain.hpp"
#include "vkit/renderer/render_pass/viewport.hpp"
#include "vkit/renderer/types.hpp"
#include "vkit/renderer/viewport.hpp"
#include "vkit/workflow/node/operators/tint.hpp"
#include "vkit/workflow/node/procedural/fractal_generator.hpp"
#include "vkit/workflow/node/procedural/noise_generator.hpp"
#include "vkit/workflow/node/texture_load.hpp"

namespace vkit {

namespace {

void registerGraphNodes(imgui::windows::ge::GraphEditorWindow& graphWindow,
                        texture::TextureManager* texManager) {
  auto& registry = graphWindow.getRegistry();
  registry.registerUI<workflow::node::TextureLoadNode>(
      std::make_unique<imgui::windows::ge::TextureLoadNodeUI>(texManager));
  registry.registerUI<workflow::node::proc::FractalGeneratorNode>(
      std::make_unique<imgui::windows::ge::FractalGeneratorNodeUI>(texManager));
  registry.registerUI<workflow::node::proc::NoiseGeneratorNode>(
      std::make_unique<imgui::windows::ge::NoiseGeneratorNodeUI>(texManager));
  registry.registerUI<workflow::node::proc::PatternGeneratorNode>(
      std::make_unique<imgui::windows::ge::PatternGeneratorNodeUI>(texManager));
  registry.registerUI<workflow::node::op::SobelNode>(
      std::make_unique<imgui::windows::ge::SobelNodeUI>(texManager));
  registry.registerUI<workflow::node::op::HeightMapNode>(
      std::make_unique<imgui::windows::ge::HeightMapNodeUI>(texManager));
  registry.registerUI<workflow::node::op::NormalMapNode>(
      std::make_unique<imgui::windows::ge::NormalMapNodeUI>(texManager));
  registry.registerUI<workflow::node::op::TintNode>(
      std::make_unique<imgui::windows::ge::TintNodeUI>(texManager));
  registry.registerUI<workflow::node::op::MixNode>(
      std::make_unique<imgui::windows::ge::MixNodeUI>(texManager));
  registry.registerUI<workflow::node::mat::PrincipledBSDFNode>(
      std::make_unique<imgui::windows::ge::PrincipledBSDFNodeUI>());
  registry.registerUI<workflow::node::mat::SlotOutputNode>(
      std::make_unique<imgui::windows::ge::SlotOutputNodeUI>());
}

void applyBlenderStyle() {
  ImGuiStyle& s = ImGui::GetStyle();

  // Spacing / sizing
  s.WindowPadding = ImVec2(8.0F, 8.0F);
  s.FramePadding = ImVec2(5.0F, 3.0F);
  s.CellPadding = ImVec2(4.0F, 2.0F);
  s.ItemSpacing = ImVec2(6.0F, 4.0F);
  s.ItemInnerSpacing = ImVec2(4.0F, 4.0F);
  s.IndentSpacing = 14.0F;
  s.ScrollbarSize = 10.0F;
  s.GrabMinSize = 10.0F;

  // Rounding
  s.WindowRounding = 4.0F;
  s.ChildRounding = 4.0F;
  s.FrameRounding = 3.0F;
  s.PopupRounding = 4.0F;
  s.ScrollbarRounding = 3.0F;
  s.GrabRounding = 3.0F;
  s.TabRounding = 3.0F;

  // Borders
  s.WindowBorderSize = 1.0F;
  s.FrameBorderSize = 0.0F;
  s.PopupBorderSize = 1.0F;
  s.ChildBorderSize = 1.0F;

  const ImVec4 k_blue = ImVec4(0.122F, 0.498F, 0.769F, 1.00F);
  const ImVec4 k_blue_dim = ImVec4(0.122F, 0.498F, 0.769F, 0.70F);
  const ImVec4 k_blue_faint = ImVec4(0.122F, 0.498F, 0.769F, 0.35F);

  ImVec4* c = s.Colors;
  c[ImGuiCol_Text] = ImVec4(0.870F, 0.870F, 0.870F, 1.00F);
  c[ImGuiCol_TextDisabled] = ImVec4(0.380F, 0.380F, 0.380F, 1.00F);
  c[ImGuiCol_WindowBg] = ImVec4(0.090F, 0.090F, 0.090F, 1.00F);
  c[ImGuiCol_ChildBg] = ImVec4(0.090F, 0.090F, 0.090F, 1.00F);
  c[ImGuiCol_PopupBg] = ImVec4(0.110F, 0.110F, 0.110F, 1.00F);
  c[ImGuiCol_Border] = ImVec4(0.055F, 0.055F, 0.055F, 1.00F);
  c[ImGuiCol_BorderShadow] = ImVec4(0.00F, 0.00F, 0.00F, 0.00F);
  c[ImGuiCol_FrameBg] = ImVec4(0.145F, 0.145F, 0.145F, 1.00F);
  c[ImGuiCol_FrameBgHovered] = ImVec4(0.185F, 0.185F, 0.185F, 1.00F);
  c[ImGuiCol_FrameBgActive] = k_blue;
  c[ImGuiCol_TitleBg] = ImVec4(0.063F, 0.063F, 0.063F, 1.00F);
  c[ImGuiCol_TitleBgActive] = ImVec4(0.090F, 0.090F, 0.090F, 1.00F);
  c[ImGuiCol_TitleBgCollapsed] = ImVec4(0.063F, 0.063F, 0.063F, 1.00F);
  c[ImGuiCol_MenuBarBg] = ImVec4(0.110F, 0.110F, 0.110F, 1.00F);
  c[ImGuiCol_ScrollbarBg] = ImVec4(0.063F, 0.063F, 0.063F, 1.00F);
  c[ImGuiCol_ScrollbarGrab] = ImVec4(0.180F, 0.180F, 0.180F, 1.00F);
  c[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.230F, 0.230F, 0.230F, 1.00F);
  c[ImGuiCol_ScrollbarGrabActive] = k_blue;
  c[ImGuiCol_CheckMark] = k_blue;
  c[ImGuiCol_SliderGrab] = ImVec4(0.230F, 0.230F, 0.230F, 1.00F);
  c[ImGuiCol_SliderGrabActive] = k_blue;
  c[ImGuiCol_Button] = ImVec4(0.180F, 0.180F, 0.180F, 1.00F);
  c[ImGuiCol_ButtonHovered] = ImVec4(0.230F, 0.230F, 0.230F, 1.00F);
  c[ImGuiCol_ButtonActive] = k_blue;
  c[ImGuiCol_Header] = ImVec4(0.155F, 0.155F, 0.155F, 1.00F);
  c[ImGuiCol_HeaderHovered] = k_blue_dim;
  c[ImGuiCol_HeaderActive] = k_blue;
  c[ImGuiCol_Separator] = ImVec4(0.055F, 0.055F, 0.055F, 1.00F);
  c[ImGuiCol_SeparatorHovered] = k_blue_dim;
  c[ImGuiCol_SeparatorActive] = k_blue;
  c[ImGuiCol_ResizeGrip] = ImVec4(0.180F, 0.180F, 0.180F, 0.50F);
  c[ImGuiCol_ResizeGripHovered] = k_blue_dim;
  c[ImGuiCol_ResizeGripActive] = k_blue;
  c[ImGuiCol_Tab] = ImVec4(0.110F, 0.110F, 0.110F, 1.00F);
  c[ImGuiCol_TabHovered] = k_blue_dim;
  c[ImGuiCol_TabActive] = ImVec4(0.155F, 0.155F, 0.155F, 1.00F);
  c[ImGuiCol_TabUnfocused] = ImVec4(0.090F, 0.090F, 0.090F, 1.00F);
  c[ImGuiCol_TabUnfocusedActive] = ImVec4(0.110F, 0.110F, 0.110F, 1.00F);
  c[ImGuiCol_DockingPreview] = k_blue_dim;
  c[ImGuiCol_DockingEmptyBg] = ImVec4(0.055F, 0.055F, 0.055F, 1.00F);
  c[ImGuiCol_PlotLines] = ImVec4(0.550F, 0.550F, 0.550F, 1.00F);
  c[ImGuiCol_PlotLinesHovered] = k_blue;
  c[ImGuiCol_PlotHistogram] = ImVec4(0.800F, 0.600F, 0.000F, 1.00F);
  c[ImGuiCol_PlotHistogramHovered] = ImVec4(1.000F, 0.700F, 0.000F, 1.00F);
  c[ImGuiCol_TableHeaderBg] = ImVec4(0.145F, 0.145F, 0.145F, 1.00F);
  c[ImGuiCol_TableBorderStrong] = ImVec4(0.055F, 0.055F, 0.055F, 1.00F);
  c[ImGuiCol_TableBorderLight] = ImVec4(0.090F, 0.090F, 0.090F, 1.00F);
  c[ImGuiCol_TableRowBg] = ImVec4(0.00F, 0.00F, 0.00F, 0.00F);
  c[ImGuiCol_TableRowBgAlt] = ImVec4(1.00F, 1.00F, 1.00F, 0.04F);
  c[ImGuiCol_TextSelectedBg] = k_blue_faint;
  c[ImGuiCol_DragDropTarget] = ImVec4(1.00F, 1.00F, 0.00F, 0.90F);
  c[ImGuiCol_NavHighlight] = k_blue;
  c[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00F, 1.00F, 1.00F, 0.70F);
  c[ImGuiCol_NavWindowingDimBg] = ImVec4(0.00F, 0.00F, 0.00F, 0.50F);
  c[ImGuiCol_ModalWindowDimBg] = ImVec4(0.00F, 0.00F, 0.00F, 0.50F);
}

};  // namespace

App::App() {
  initWindow();
  initVulkan();
  initCore();
  initImguiCore();
  initCompute();
  initEnvironment();
  initAsset();
  initWorkflow();
  initRendererSystems();
  initViewports();
  initImguiWindows();
}

App::~App() {
  if (sys_.device) {
    sys_.device->get().waitIdle();
  }

  for (auto& frame : frames_) {
    if (frame.sceneTextureId)
      ui_.renderer->unregisterTexture(frame.sceneTextureId);
    if (frame.materialTextureId)
      ui_.renderer->unregisterTexture(frame.materialTextureId);
  }

  ui_.windowManager.reset();
  ui_.graphWindow.reset();
  ui_.graphInspector.reset();
  ui_.configWindow.reset();
  ui_.sceneViewer.reset();
  ui_.materialViewer.reset();

  engine_.workflowController.reset();
  engine_.workflow.reset();
  engine_.assetController.reset();
  engine_.assetManager.reset();
  engine_.environmentController.reset();
  engine_.environmentManager.reset();

  if (engine_.textureManager) {
    engine_.textureManager->setImguiRenderer(nullptr);
  }
}

void App::initWindow() {
  sys_.glfw = std::make_unique<window::Context>();
  const auto config = window::WindowConfiguration{
      .show = true, .fullscreen = false, .size = {1280, 720}, .title = "vkit"};
  sys_.window = std::make_unique<window::Window>(config);
}

void App::initVulkan() {
  sys_.instance = std::make_unique<graphics::Instance>();
  sys_.surface =
      std::make_unique<graphics::Surface>(*sys_.window, *sys_.instance);
  sys_.device =
      std::make_unique<graphics::GfxDevice>(*sys_.instance, *sys_.surface);

  sys_.swapchain = std::make_unique<graphics::Swapchain>(
      *sys_.device, sys_.surface->get(), kMaxFramesInFlight,
      glm::ivec2{1280, 720});

  sys_.renderer = std::make_unique<renderer::Renderer>(
      sys_.device->get(), sys_.device->queues.graphicsPresent,
      sys_.device->queueFamilies.graphicsPresent, kMaxFramesInFlight);

  sys_.imageAvailableSemaphores.reserve(kMaxFramesInFlight);
  for (int i = 0; i < kMaxFramesInFlight; ++i) {
    sys_.imageAvailableSemaphores.push_back(
        sys_.device->get().createSemaphoreUnique({}));
  }

  const auto image_count = sys_.swapchain->getImageCount();
  sys_.renderFinishedSemaphores.reserve(image_count);
  for (std::uint32_t i = 0; i < image_count; ++i) {
    sys_.renderFinishedSemaphores.push_back(
        sys_.device->get().createSemaphoreUnique({}));
  }
}

void App::initCore() {
  using BindlessDSL = renderer::dsl::BindlessTextureSetLayout;
  sys_.bindlessSetLayout = std::make_unique<BindlessDSL>(sys_.device->get());

  std::array<vk::DescriptorPoolSize, 2> pool_sizes = {
      vk::DescriptorPoolSize{vk::DescriptorType::eSampler,
                             BindlessDSL::kMaxSamplers},
      vk::DescriptorPoolSize{vk::DescriptorType::eSampledImage,
                             BindlessDSL::kMaxTextures}};

  vk::DescriptorPoolCreateInfo pool_info{};
  pool_info.setFlags(vk::DescriptorPoolCreateFlagBits::eUpdateAfterBind)
      .setMaxSets(1)
      .setPoolSizes(pool_sizes);

  sys_.bindlessPool = sys_.device->get().createDescriptorPoolUnique(pool_info);

  auto layout_handle = sys_.bindlessSetLayout->get();
  vk::DescriptorSetAllocateInfo alloc_info{};
  alloc_info.setDescriptorPool(sys_.bindlessPool.get())
      .setDescriptorSetCount(1)
      .setSetLayouts(layout_handle);

  sys_.bindlessSet =
      sys_.device->get().allocateDescriptorSets(alloc_info).front();

  static constexpr std::array<std::uint8_t, 4> kRedPixel = {255, 0, 0, 255};
  auto red_span = std::span<const std::byte>{
      reinterpret_cast<const std::byte*>(kRedPixel.data()), kRedPixel.size()};

  texture::LoadOptions options{};
  options.useMipmaps = false;
  options.isSrgb = false;

  auto loaded_dummy = texture::loadFromRawPixels(
      sys_.device->get(), sys_.device->allocator, red_span, 1, 1, options);

  engine_.dummyTexture = loaded_dummy.texture;

  const auto submit_info = graphics::util::RecordAndSubmitInfo{
      .device = sys_.device->get(),
      .queue = sys_.device->queues.graphicsPresent,
      .commandPool = sys_.device->getGraphicsPresentCommandPool(),
  };

  graphics::util::recordAndSubmit(submit_info, [&](vk::CommandBuffer cb) {
    engine_.dummyTexture->recordUpload(cb, loaded_dummy.stagingBuffer);
  });

  engine_.bindlessManager = std::make_unique<graphics::BindlessTextureManager>(
      sys_.device->get(), sys_.bindlessSet, BindlessDSL::kMaxSamplers,
      BindlessDSL::kMaxTextures, engine_.dummyTexture);

  engine_.textureManager =
      std::make_shared<texture::TextureManager>(kMaxFramesInFlight);
  engine_.textureManager->setBindlessManager(engine_.bindlessManager.get());

  engine_.materialManager = std::make_unique<material::MaterialManager>();
}

void App::initImguiCore() {
  ui_.renderer = std::make_unique<imgui::ImguiRenderer>(
      sys_.device->get(), sys_.device->allocator, sys_.swapchain->getFormat(),
      vk::SampleCountFlagBits::e1, kMaxFramesInFlight);

  engine_.textureManager->setImguiRenderer(ui_.renderer.get());

  ui_.host = std::make_unique<imgui::WindowImguiHost>(
      sys_.window.get(), *ui_.renderer, "vkit", "");

  applyBlenderStyle();

  ui_.host->setDockLayoutCallback([](vkit::imgui::WindowImguiHost& host,
                                     ImVec2 availableSize) {
    ImGuiID root_id = host.getRootDockId();
    if (ImGui::DockBuilderGetNode(root_id) == nullptr ||
        ImGui::DockBuilderGetNode(root_id)->ChildNodes[0] == nullptr) {
      ImGui::DockBuilderRemoveNode(root_id);
      ImGui::DockBuilderAddNode(root_id, ImGuiDockNodeFlags_DockSpace);
      ImGui::DockBuilderSetNodeSize(root_id, availableSize);

      ImGuiID dock_main_id = root_id;
      ImGuiID dock_bottom_id = ImGui::DockBuilderSplitNode(
          dock_main_id, ImGuiDir_Down, 0.40F, nullptr, &dock_main_id);
      ImGuiID dock_top_right_id = ImGui::DockBuilderSplitNode(
          dock_main_id, ImGuiDir_Right, 0.30F, nullptr, &dock_main_id);
      ImGuiID dock_top_right_bottom_id = ImGui::DockBuilderSplitNode(
          dock_top_right_id, ImGuiDir_Down, 0.40F, nullptr, &dock_top_right_id);
      ImGuiID dock_bottom_left_id = ImGui::DockBuilderSplitNode(
          dock_bottom_id, ImGuiDir_Left, 0.20F, nullptr, &dock_bottom_id);

      ImGui::DockBuilderDockWindow("Scene", dock_main_id);
      ImGui::DockBuilderDockWindow("Graph Editor", dock_bottom_id);
      ImGui::DockBuilderDockWindow("Graph Node Inspector", dock_bottom_left_id);
      ImGui::DockBuilderDockWindow("Material Preview", dock_top_right_id);
      ImGui::DockBuilderDockWindow("Configuration", dock_top_right_bottom_id);

      ImGui::DockBuilderFinish(root_id);
    }
  });

  {
    auto& io = ImGui::GetIO();
    ImFontConfig cfg;
    cfg.OversampleH = 2;
    cfg.OversampleV = 2;
    io.Fonts->AddFontFromFileTTF(
        asset::assetPath("fonts/Roboto-Medium.ttf").string().c_str(), 15.0F,
        &cfg);
  }

  const auto submit_info = graphics::util::RecordAndSubmitInfo{
      .device = sys_.device->get(),
      .queue = sys_.device->queues.graphicsPresent,
      .commandPool = sys_.device->getGraphicsPresentCommandPool(),
  };

  ui_.renderer->uploadFont(submit_info);
  ui_.windowManager = std::make_unique<imgui::ImguiWindowManager>();
}

void App::initCompute() {
  engine_.executionContext = std::make_shared<workflow::ExecutionContext>();

  engine_.textureUploader = std::make_shared<texture::TextureUploader>(
      *sys_.device, engine_.textureManager,
      engine_.executionContext->texLoadBus,
      engine_.executionContext->texReadyBus);

  engine_.asyncCompute = std::make_shared<compute::AsyncCompute>(
      sys_.device->get(), sys_.device->queues.compute,
      sys_.device->queueFamilies.compute);

  using BL = core::events::ComputeBindingLayout;
  engine_.computeOutputDispatcher =
      std::make_shared<compute::ComputeOutputDispatcher>(
          *sys_.device, engine_.executionContext->computeOutputBus,
          engine_.executionContext->computeOutputResultBus,
          engine_.asyncCompute);

  auto& d = *engine_.computeOutputDispatcher;
  auto dev = sys_.device->get();
  d.registerPipeline("fractal", dev,
                     shaders::shaderPath(shaders::kProceduralFractalShaderPath),
                     BL::kGenerator,
                     sizeof(core::events::FractalPushConstants));
  d.registerPipeline("noise", dev,
                     shaders::shaderPath(shaders::kProceduralNoiceShaderPath),
                     BL::kGenerator, sizeof(core::events::NoisePushConstants));
  d.registerPipeline("pattern", dev,
                     shaders::shaderPath(shaders::kProceduralPatternShaderPath),
                     BL::kGenerator,
                     sizeof(core::events::PatternPushConstants));
  d.registerPipeline(
      "sobel", dev, shaders::shaderPath(shaders::kOperatorsSobelShaderPath),
      BL::kSingleInput, sizeof(core::events::SobelPushConstants));
  d.registerPipeline(
      "heightmap", dev,
      shaders::shaderPath(shaders::kOperatorsHeightMapShaderPath),
      BL::kSingleInput, sizeof(core::events::HeightMapPushConstants));
  d.registerPipeline(
      "normalmap", dev,
      shaders::shaderPath(shaders::kOperatorsNormalMapShaderPath),
      BL::kSingleInput, sizeof(core::events::NormalMapPushConstants));
  d.registerPipeline("tint", dev,
                     shaders::shaderPath(shaders::kOperatorsTintShaderPath),
                     BL::kSingleInput, sizeof(core::events::TintPushConstants));
  d.registerPipeline("mix", dev,
                     shaders::shaderPath(shaders::kOperatorsMixShaderPath),
                     BL::kDualInput, sizeof(core::events::MixPushConstants));
}

void App::initEnvironment() {
  engine_.environmentManager = std::make_shared<env::EnvironmentManager>(
      *sys_.device, engine_.textureManager, engine_.asyncCompute);

  engine_.environmentManager->initializeGlobalBrdfLut();

  engine_.environmentController =
      std::make_unique<controller::EnvironmentController>(
          engine_.environmentManager.get());
}

void App::initAsset() {
  engine_.gltfStorage = std::make_shared<asset::GltfStorage>();
  engine_.assetManager = std::make_shared<asset::AssetManager>(
      *sys_.device, engine_.gltfStorage, kMaxFramesInFlight);
  engine_.assetController =
      std::make_unique<controller::AssetController>(engine_.assetManager.get());

  std::filesystem::path asset_path =
      asset::assetPath("models/mechdrone/scene.gltf");
  if (std::filesystem::exists(asset_path)) {
    auto default_asset = engine_.assetManager->loadGltfAsset(asset_path);
    if (default_asset) {
      engine_.assetController->setCurrentAsset(
          default_asset->getStorageId().value());
    }
  }
}

void App::initWorkflow() {
  engine_.workflow = std::make_unique<workflow::Workflow>();

  engine_.workflowController =
      std::make_unique<controller::WorkflowController>();
  engine_.workflowController->setWorkflow(engine_.workflow.get())
      .setTextureManager(engine_.textureManager.get())
      .setExecutionContext(engine_.executionContext.get())
      .setComputeDispatcher(engine_.computeOutputDispatcher.get())
      .setMaterialManager(engine_.materialManager.get());
}

void App::initViewports() {
  scene_.sceneCamera = std::make_shared<scene::Camera>("Scene Camera");
  scene_.sceneCamera->setPosition(glm::vec3(0.0F, 2.0F, 5.0F));

  // Default camera-attached point light (position updated every frame).
  ui_.cameraLight.type = static_cast<int>(renderer::types::LightType::kPoint);
  ui_.cameraLight.color = {1.0F, 1.0F, 1.0F};
  ui_.cameraLight.intensity = 1.5F;
  ui_.cameraLight.range = 25.0F;
  ui_.cameraLight.castsShadows = 0;

  scene_.materialCamera = std::make_shared<scene::Camera>("Material Camera");
  scene_.materialController.target = glm::vec3(0.0F, 0.0F, 0.0F);
  scene_.materialController.distance = 2.5F;

  ui_.sceneViewer = std::make_shared<imgui::windows::Viewer>("Scene");
  ui_.sceneViewer->setOnResize(
      [this](std::uint32_t width, std::uint32_t height) {
        if (width > 0 && height > 0) {
          scene_.sceneCamera->setAspectRatio(static_cast<float>(width) /
                                             static_cast<float>(height));
        }
      });

  ui_.sceneViewer->setOnManipulate(
      [this](imgui::windows::Viewer& viewer) mutable {
        auto& io = ImGui::GetIO();
        if (viewer.isHovered() && viewer.isFocused()) {
          glm::vec3 local_dir{0.0F, 0.0F, 0.0F};
          if (ImGui::IsKeyDown(ImGuiKey_W)) local_dir.z += 1.0F;
          if (ImGui::IsKeyDown(ImGuiKey_S)) local_dir.z -= 1.0F;
          if (ImGui::IsKeyDown(ImGuiKey_D)) local_dir.x += 1.0F;
          if (ImGui::IsKeyDown(ImGuiKey_A)) local_dir.x -= 1.0F;
          if (ImGui::IsKeyDown(ImGuiKey_E)) local_dir.y += 1.0F;
          if (ImGui::IsKeyDown(ImGuiKey_Q)) local_dir.y -= 1.0F;

          if (glm::length(local_dir) > 0.0F) {
            local_dir = glm::normalize(local_dir);
            float speed_mult = (ImGui::IsKeyDown(ImGuiKey_LeftShift) ||
                                ImGui::IsKeyDown(ImGuiKey_RightShift))
                                   ? 3.0F
                                   : 1.0F;
            scene_.sceneController.move(local_dir, io.DeltaTime * speed_mult);
          }
        }

        if (viewer.isHovered() && ImGui::IsMouseDown(ImGuiMouseButton_Right)) {
          scene_.sceneController.processMouseMovement(io.MouseDelta.x,
                                                      io.MouseDelta.y);
        }

        if (viewer.isHovered() && io.MouseWheel != 0.0F) {
          float new_speed =
              scene_.sceneController.movementSpeed + (io.MouseWheel * 2.0F);
          scene_.sceneController.setMovementSpeed(std::max(0.1F, new_speed));
        }
      });

  ui_.materialViewer =
      std::make_shared<imgui::windows::Viewer>("Material Preview");
  ui_.materialViewer->setOnResize(
      [this](std::uint32_t width, std::uint32_t height) {
        if (width > 0 && height > 0) {
          scene_.materialCamera->setAspectRatio(static_cast<float>(width) /
                                                static_cast<float>(height));
        }
      });

  ui_.materialViewer->setOnManipulate([this](imgui::windows::Viewer& viewer) {
    auto& io = ImGui::GetIO();
    if (viewer.isHovered() && viewer.isFocused()) {
      if (ImGui::IsMouseDown(ImGuiMouseButton_Left) ||
          ImGui::IsMouseDown(ImGuiMouseButton_Right)) {
        scene_.materialController.processMouseMovement(io.MouseDelta.x,
                                                       io.MouseDelta.y);
      }
      if (io.MouseWheel != 0.0F) {
        scene_.materialController.processScroll(io.MouseWheel);
      }
    }
  });

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

void App::initImguiWindows() {
  ui_.graphWindow =
      std::make_shared<imgui::windows::ge::GraphEditorWindow>("Graph Editor");
  ui_.graphWindow->setController(engine_.workflowController.get());

  registerGraphNodes(*ui_.graphWindow, engine_.textureManager.get());

  ui_.graphInspector =
      std::make_shared<imgui::windows::ge::GraphNodeInspectorWindow>(
          "Graph Node Inspector", ui_.graphWindow.get());

  ui_.configWindow =
      std::make_shared<imgui::windows::ConfigurationWindow>("Configuration");
  ui_.configWindow->setAssetController(engine_.assetController.get());
  ui_.configWindow->setEnvironmentController(
      engine_.environmentController.get());
  ui_.configWindow->setAnimator(rSys_.animator.get());
  ui_.configWindow->setEnableSkinning(&ui_.enableSkinning);
  ui_.configWindow->setSceneParams(&ui_.sceneParams);
  ui_.configWindow->setLights(&ui_.lights);
  ui_.configWindow->setMaterialPreviewData(engine_.materialManager.get(),
                                           &ui_.previewMaterialSlot);
  ui_.configWindow->setWorkflowController(engine_.workflowController.get());
  ui_.configWindow->setSelectedPrimitive(&ui_.selectedPrimitive);
  ui_.configWindow->setCameraLight(&ui_.cameraLight, &ui_.cameraLightEnabled);

  ui_.windowManager->addWindow(ui_.sceneViewer);
  ui_.windowManager->addWindow(ui_.materialViewer);
  ui_.windowManager->addWindow(ui_.graphWindow);
  ui_.windowManager->addWindow(ui_.graphInspector);
  ui_.windowManager->addWindow(ui_.configWindow);
}

void App::initRendererSystems() {
  rSys_.sceneRenderer = std::make_unique<renderer::SceneRenderer>(
      *sys_.device, *sys_.bindlessSetLayout, kMaxFramesInFlight);

  rSys_.materialSys = std::make_unique<renderer::MaterialSystem>(
      *sys_.device, rSys_.sceneRenderer->getMaterialSetLayout(),
      kMaxFramesInFlight);

  rSys_.assetBridge = std::make_unique<renderer::AssetRenderBridge>(
      *sys_.device, rSys_.sceneRenderer->getPrimitiveSetLayout(),
      kMaxFramesInFlight);

  rSys_.animator = std::make_unique<animation::Animator>();
}

void App::ensureViewportSize(renderer::Viewport& viewport,
                             ImTextureID& textureId, std::uint32_t width,
                             std::uint32_t height,
                             std::uint32_t displayTargetIndex) const {
  const auto target_width = std::max(1U, width);
  const auto target_height = std::max(1U, height);

  const bool resized = viewport.ensureSize(
      sys_.device->get(), sys_.device->allocator, target_width, target_height);

  // Only update the descriptor set when the view was recreated or first
  // created. Updating every frame races against in-flight command buffers that
  // hold the set.
  if ((resized || textureId == 0) &&
      viewport.colorTargets.size() > displayTargetIndex &&
      viewport.colorTargets[displayTargetIndex].view) {
    textureId = ui_.renderer->updateOrRegisterTexture(
        textureId, *viewport.colorTargets[displayTargetIndex].view);
  }
}

void App::recreateSwapchain() {
  auto width = sys_.window->getWidth();
  auto height = sys_.window->getHeight();

  if (width > 0 && height > 0) {
    sys_.device->get().waitIdle();
    sys_.swapchain->recreate(glm::ivec2{width, height});

    const auto image_count = sys_.swapchain->getImageCount();
    if (image_count != sys_.renderFinishedSemaphores.size()) {
      sys_.renderFinishedSemaphores.clear();
      sys_.renderFinishedSemaphores.reserve(image_count);
      for (std::uint32_t i = 0; i < image_count; ++i) {
        sys_.renderFinishedSemaphores.push_back(
            sys_.device->get().createSemaphoreUnique({}));
      }
    }
  }
}

void App::run() { mainLoop(); }

void App::mainLoop() {
  auto last_time = std::chrono::steady_clock::now();
  auto swapchain_needs_recreation = false;

  while (!sys_.window->shouldClose()) {
    sys_.window->pollEvents();

    engine_.update();

    auto current_time = std::chrono::steady_clock::now();
    float dt = std::chrono::duration<float, std::chrono::seconds::period>(
                   current_time - last_time)
                   .count();
    last_time = current_time;

    for (auto& event : sys_.window->getInputEvents()) {
      ui_.host->dispatchInputEvent(event);
      if (event.type == window::InputEventType::kWindowResizeEvent) {
        swapchain_needs_recreation = true;
      }
    }

    if (swapchain_needs_recreation) {
      recreateSwapchain();
      swapchain_needs_recreation = false;
      continue;
    }

    sys_.renderer->beginFrame(currentFrame_);

    engine_.assetManager->processGC();
    engine_.textureManager->processGC();

    auto image_index_opt = sys_.swapchain->acquireNextImage(
        *sys_.imageAvailableSemaphores[currentFrame_]);
    if (!image_index_opt) {
      swapchain_needs_recreation = true;
      continue;
    }

    const auto image_index = image_index_opt.value();
    auto& frame = frames_[currentFrame_];

    if (ui_.sceneViewer->isVisible()) {
      ensureViewportSize(frame.sceneViewport, frame.sceneTextureId,
                         ui_.sceneViewer->getWidth(),
                         ui_.sceneViewer->getHeight(), 1);
      ui_.sceneViewer->setCurrentTexture(frame.sceneTextureId);
      scene_.sceneController.update(*scene_.sceneCamera);
    }

    if (ui_.materialViewer->isVisible()) {
      ensureViewportSize(frame.materialViewport, frame.materialTextureId,
                         ui_.materialViewer->getWidth(),
                         ui_.materialViewer->getHeight(), 1);
      ui_.materialViewer->setCurrentTexture(frame.materialTextureId);
      scene_.materialController.update(*scene_.materialCamera);
    }

    auto current_asset = engine_.assetController->getCurrentAsset();
    if (current_asset) {
      current_asset->update();
    }

    if (ui_.enableSkinning)
      rSys_.animator->update(dt, current_asset.get());
    rSys_.materialSys->update(currentFrame_, *engine_.materialManager);
    rSys_.assetBridge->update(currentFrame_, current_asset.get());

    env::EnvironmentParams env_params{};
    auto current_env = engine_.environmentController->getCurrentEnvironment();
    glm::vec4 env_base_color = glm::vec4(0.2F, 0.2F, 0.2F, 1.0F);
    float env_blur = 0.0F;

    if (current_env) {
      env_params = current_env->getData();
      env_blur = current_env->blurLevel;
      env_base_color = glm::vec4(1.0F);
    } else {
      env_params.intensity = 1.0F;
      env_params.prefilterTexIdx = -1;
      env_params.irradianceTexIdx = -1;
      env_params.brdfLutTexIdx = -1;
    }

    renderer::types::CameraUBO scene_ubo{
        .view = scene_.sceneCamera->getViewMatrix(),
        .proj = scene_.sceneCamera->getProjectionMatrix(),
        .position = scene_.sceneCamera->getPosition(),
    };
    renderer::types::CameraUBO mat_ubo{
        .view = scene_.materialCamera->getViewMatrix(),
        .proj = scene_.materialCamera->getProjectionMatrix(),
        .position = scene_.materialCamera->getPosition(),
    };

    // Build LightsSSBO from the configured lights list plus optional camera light.
    renderer::types::LightsSSBO lights_ssbo{};
    std::uint32_t light_count = 0;
    for (const auto& l : ui_.lights) {
      if (light_count >= renderer::types::kMaxSceneLights) break;
      lights_ssbo.lights[light_count++] = l;
    }
    if (ui_.cameraLightEnabled &&
        light_count < renderer::types::kMaxSceneLights) {
      ui_.cameraLight.position = scene_.sceneCamera->getPosition();
      lights_ssbo.lights[light_count++] = ui_.cameraLight;
    }
    lights_ssbo.count = light_count;

    // Build shadow light UBO from the first shadow-casting light.
    renderer::types::ShadowLightUBO shadow_light{};
    bool any_shadow_caster = false;
    static constexpr float kShadowArea     = 20.0F;
    static constexpr float kShadowDistance = 50.0F;
    static constexpr float kPi             = 3.14159265F;
    for (std::uint32_t li = 0; li < lights_ssbo.count; ++li) {
      const auto& sl = lights_ssbo.lights[li];
      if (sl.castsShadows == 0) continue;
      any_shadow_caster = true;

      const glm::vec3 ldir = glm::normalize(sl.direction);
      const glm::vec3 up =
          (std::abs(glm::dot(ldir, glm::vec3(0.0F, 1.0F, 0.0F))) < 0.99F)
              ? glm::vec3(0.0F, 1.0F, 0.0F)
              : glm::vec3(1.0F, 0.0F, 0.0F);

      glm::mat4 lview, lproj;
      const float half = kShadowArea * 0.5F;

      if (sl.type == static_cast<int>(renderer::types::LightType::kDirectional)) {
        // Orthographic, looking from virtual light position toward scene center.
        const glm::vec3 lpos = -ldir * kShadowDistance;
        lview = glm::lookAt(lpos, glm::vec3(0.0F), up);
        lproj = glm::orthoRH_ZO(-half, half, -half, half, 0.1F, kShadowDistance * 2.0F);
      } else if (sl.type == static_cast<int>(renderer::types::LightType::kSpot)) {
        // Perspective frustum aligned with the spot's direction.
        const glm::vec3 lpos = sl.position;
        lview = glm::lookAt(lpos, lpos + ldir, up);
        const float fov = glm::clamp(sl.outerAngle * 2.0F, 0.01F, kPi * 0.99F);
        lproj = glm::perspectiveRH_ZO(fov, 1.0F, 0.1F, sl.range * 2.0F);
      } else {
        // Point light: orthographic toward scene center (single shadow map approximation).
        const glm::vec3 lpos = sl.position;
        lview = glm::lookAt(lpos, glm::vec3(0.0F), up);
        lproj = glm::orthoRH_ZO(-half, half, -half, half, 0.1F, sl.range * 2.0F);
      }

      shadow_light.viewProj = lproj * lview;
      ui_.sceneParams.shadowViewProj = shadow_light.viewProj;
      break;
    }

    rSys_.sceneRenderer->updateUniforms(currentFrame_, scene_ubo, mat_ubo,
                                        env_params, ui_.sceneParams,
                                        lights_ssbo, shadow_light);

    ui_.host->beginFrame(sys_.window->getWidth(), sys_.window->getHeight(), dt);
    ui_.windowManager->drawWindows(currentFrame_);
    ui_.host->endFrame();

    auto task = renderer::RenderTask{};

    auto scene_set = rSys_.sceneRenderer->getSceneDescriptorSet(currentFrame_);
    auto mat_scene_set =
        rSys_.sceneRenderer->getMaterialDescriptorSet(currentFrame_);
    auto mat_set = rSys_.materialSys->getDescriptorSet(currentFrame_);
    auto prim_set = rSys_.assetBridge->getDescriptorSet(currentFrame_);
    auto light_set = rSys_.sceneRenderer->getLightDescriptorSet(currentFrame_);

    // Shadow pass: only when shadows are enabled and a shadow-casting light exists.
    if (current_asset && ui_.sceneParams.shadowsEnabled && any_shadow_caster) {
      task.add<renderer::rp::BeginShadowPass>(
              rSys_.sceneRenderer->getShadowImage(),
              rSys_.sceneRenderer->getShadowImageView(),
              rSys_.sceneRenderer->getShadowExtent())
          .add<renderer::cmd::DrawShadowCommand>(
              rSys_.sceneRenderer->shadowPipeline,
              rSys_.sceneRenderer->shadowLayout->get(), light_set, prim_set,
              current_asset.get(), engine_.materialManager.get(),
              ui_.enableSkinning)
          .add<renderer::rp::EndShadowPass>(
              rSys_.sceneRenderer->getShadowImage());
    }

    if (ui_.sceneViewer->isVisible()) {
      task.add<renderer::rp::BeginViewportPass>(frame.sceneViewport, 0, 1)
          .add<renderer::cmd::DrawSkyboxCommand>(
              rSys_.sceneRenderer->skyboxPipeline,
              rSys_.sceneRenderer->skyboxLayout->get(), scene_set,
              sys_.bindlessSet, env_base_color, env_blur)
          .add<renderer::cmd::DrawAssetCommand>(
              rSys_.sceneRenderer->opaquePipeline,
              rSys_.sceneRenderer->transparentPipeline,
              rSys_.sceneRenderer->transparentBackFacePipeline,
              rSys_.sceneRenderer->primitiveLayout->get(), scene_set,
              sys_.bindlessSet, mat_set, prim_set, current_asset.get(),
              engine_.materialManager.get(), ui_.enableSkinning,
              scene_.sceneCamera->getPosition())
          .add<renderer::cmd::DrawOutlineCommand>(
              rSys_.sceneRenderer->outlinePipeline,
              rSys_.sceneRenderer->primitiveLayout->get(), scene_set,
              sys_.bindlessSet, mat_set, prim_set, current_asset.get(),
              ui_.enableSkinning, ui_.selectedPrimitive)
          .add<renderer::cmd::DrawLightGizmosCommand>(
              rSys_.sceneRenderer->lightGizmoPipeline,
              rSys_.sceneRenderer->lightGizmoLayout->get(), scene_set,
              lights_ssbo.count)
          .add<renderer::rp::EndViewportPass>(frame.sceneViewport, 1);
    }

    if (ui_.materialViewer->isVisible()) {
      if (ui_.materialViewer->isVisible()) {
        task.add<renderer::rp::BeginViewportPass>(frame.materialViewport, 0, 1)
            .add<renderer::cmd::DrawSkyboxCommand>(
                rSys_.sceneRenderer->skyboxPipeline,
                rSys_.sceneRenderer->skyboxLayout->get(), mat_scene_set,
                sys_.bindlessSet, env_base_color, env_blur)
            .add<renderer::cmd::DrawRaySphereCommand>(
                rSys_.sceneRenderer->opaqueRaySpherePipeline,
                rSys_.sceneRenderer->transparentRaySpherePipeline,
                rSys_.sceneRenderer->transparentRaySphereBackFacePipeline,
                rSys_.sceneRenderer->raySphereLayout->get(), mat_scene_set,
                sys_.bindlessSet, mat_set, glm::mat4(1.0F),
                ui_.previewMaterialSlot, engine_.materialManager.get())
            .add<renderer::rp::EndViewportPass>(frame.materialViewport, 1);
      }
    }

    task.add<renderer::rp::BeginSwapchainPass>(
            sys_.swapchain->getImage(image_index),
            sys_.swapchain->getImageView(image_index),
            sys_.swapchain->getExtent())
        .add<renderer::cmd::DrawImGuiCommand>(*ui_.host, currentFrame_)
        .add<renderer::rp::EndSwapchainPass>(
            sys_.swapchain->getImage(image_index));

    auto& render_finished = *sys_.renderFinishedSemaphores[image_index];
    auto result = sys_.renderer->submit(
        currentFrame_, task, *sys_.imageAvailableSemaphores[currentFrame_],
        render_finished);

    const auto presented =
        sys_.swapchain->present(image_index, result.renderFinishedSemaphore);
    if (!presented) {
      swapchain_needs_recreation = true;
    }

    currentFrame_ = (currentFrame_ + 1) % kMaxFramesInFlight;
  }
}

};  // namespace vkit
