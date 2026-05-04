#pragma once

#include <array>
#include <memory>
#include <vector>
#include <vk_mem_alloc.hpp>

#include "vkit/animation/animator.hpp"
#include "vkit/asset/asset_manager.hpp"
#include "vkit/asset/gltf_storage.hpp"
#include "vkit/compute/async_compute.hpp"
#include "vkit/compute/heightmap_dispatcher.hpp"
#include "vkit/compute/mix_dispatcher.hpp"
#include "vkit/compute/noise_dispatcher.hpp"
#include "vkit/compute/normalmap_dispatcher.hpp"
#include "vkit/compute/sobel_dispatcher.hpp"
#include "vkit/compute/tint_dispatcher.hpp"
#include "vkit/controller/asset.hpp"
#include "vkit/controller/camera.hpp"
#include "vkit/controller/environment.hpp"
#include "vkit/controller/workflow.hpp"
#include "vkit/environment/manager.hpp"
#include "vkit/graphics/device.hpp"
#include "vkit/graphics/instance.hpp"
#include "vkit/graphics/surface.hpp"
#include "vkit/graphics/swapchain.hpp"
#include "vkit/imgui/imgui_renderer.hpp"
#include "vkit/imgui/imgui_window_manager.hpp"
#include "vkit/imgui/window_imgui_host.hpp"
#include "vkit/imgui/windows/configuration.hpp"
#include "vkit/imgui/windows/ge/graph_editor.hpp"
#include "vkit/imgui/windows/ge/graph_node_inspector.hpp"
#include "vkit/imgui/windows/viewer.hpp"
#include "vkit/material/manager.hpp"
#include "vkit/platform/file_dialog.hpp"
#include "vkit/renderer/asset_render_bridge.hpp"
#include "vkit/renderer/descriptor_set_layout/bindless.hpp"
#include "vkit/renderer/material_system.hpp"
#include "vkit/renderer/renderer.hpp"
#include "vkit/renderer/scene_renderer.hpp"
#include "vkit/renderer/viewport.hpp"
#include "vkit/scene/camera.hpp"
#include "vkit/texture/manager.hpp"
#include "vkit/texture/uploader.hpp"
#include "vkit/workflow/execution_context.hpp"
#include "vkit/workflow/workflow.hpp"

namespace vkit {

class App {
 public:
  App();
  ~App();

  void run();

 private:
  void initWindow();
  void initVulkan();

  void initCore();
  void initImguiCore();
  void initCompute();
  void initEnvironment();
  void initAsset();
  void initWorkflow();
  void initRendererSystems();
  void initViewports();
  void initImguiWindows();

  void mainLoop();
  void recreateSwapchain() const;

  static constexpr int kMaxFramesInFlight = 3;
  std::uint32_t currentFrame_{0};

  platform::FileDialogContext fileDialog_;

  struct SystemContext {
    std::unique_ptr<window::Context> glfw;
    std::unique_ptr<window::Window> window;
    std::unique_ptr<graphics::Instance> instance;
    std::unique_ptr<graphics::Surface> surface;
    std::unique_ptr<graphics::GfxDevice> device;
    std::unique_ptr<graphics::Swapchain> swapchain;
    std::unique_ptr<renderer::Renderer> renderer;
    std::vector<vk::UniqueSemaphore> imageAvailableSemaphores;

    std::unique_ptr<renderer::dsl::BindlessTextureSetLayout> bindlessSetLayout;
    vk::UniqueDescriptorPool bindlessPool;
    vk::DescriptorSet bindlessSet;
  } sys_;

  struct EngineContext {
    std::shared_ptr<graphics::Texture> dummyTexture;
    std::unique_ptr<graphics::BindlessTextureManager> bindlessManager;
    std::shared_ptr<texture::TextureManager> textureManager;
    std::shared_ptr<texture::TextureUploader> textureUploader;
    std::shared_ptr<compute::AsyncCompute> asyncCompute;

    std::shared_ptr<workflow::ExecutionContext> executionContext;
    std::shared_ptr<compute::NoiseDispatcher> noiseDispatcher;
    std::shared_ptr<compute::SobelDispatcher> sobelDispatcher;
    std::shared_ptr<compute::HeightMapDispatcher> heightMapDispatcher;
    std::shared_ptr<compute::NormalMapDispatcher> normalMapDispatcher;
    std::shared_ptr<compute::TintDispatcher> tintDispatcher;
    std::shared_ptr<compute::MixDispatcher> mixDispatcher;

    std::shared_ptr<asset::GltfStorage> gltfStorage;
    std::shared_ptr<asset::AssetManager> assetManager;
    std::unique_ptr<controller::AssetController> assetController;
    std::shared_ptr<env::EnvironmentManager> environmentManager;
    std::unique_ptr<controller::EnvironmentController> environmentController;
    std::unique_ptr<material::MaterialManager> materialManager;
    std::unique_ptr<workflow::Workflow> workflow;
    std::unique_ptr<controller::WorkflowController> workflowController;

    void update() {
      if (workflow) workflow->execute();
      if (executionContext) executionContext->update();
      if (textureUploader) textureUploader->update();

      if (noiseDispatcher) noiseDispatcher->update();
      if (sobelDispatcher) sobelDispatcher->update();
      if (heightMapDispatcher) heightMapDispatcher->update();
      if (normalMapDispatcher) normalMapDispatcher->update();
      if (tintDispatcher) tintDispatcher->update();
      if (mixDispatcher) mixDispatcher->update();

      if (materialManager) materialManager->update();
    }
  } engine_;

  struct RenderSystems {
    std::unique_ptr<animation::Animator> animator;
    std::unique_ptr<renderer::MaterialSystem> materialSys;
    std::unique_ptr<renderer::AssetRenderBridge> assetBridge;
    std::unique_ptr<renderer::SceneRenderer> sceneRenderer;
  } rSys_;

  struct UIContext {
    std::unique_ptr<imgui::ImguiRenderer> renderer;
    std::unique_ptr<imgui::WindowImguiHost> host;
    std::unique_ptr<imgui::ImguiWindowManager> windowManager;

    std::shared_ptr<imgui::windows::Viewer> sceneViewer;
    std::shared_ptr<imgui::windows::Viewer> materialViewer;
    std::shared_ptr<imgui::windows::ge::GraphEditorWindow> graphWindow;
    std::shared_ptr<imgui::windows::ge::GraphNodeInspectorWindow>
        graphInspector;

    std::shared_ptr<imgui::windows::ConfigurationWindow> configWindow;
  } ui_;

  struct SceneContext {
    std::shared_ptr<scene::Camera> sceneCamera;
    std::shared_ptr<scene::Camera> materialCamera;
    controller::FreeCameraController sceneController;
    controller::OrbitalCameraController materialController;
  } scene_;

  struct Frame {
    renderer::Viewport sceneViewport;
    ImTextureID sceneTextureId{0};

    renderer::Viewport materialViewport;
    ImTextureID materialTextureId{0};
  };

  std::array<Frame, kMaxFramesInFlight> frames_;

  void ensureViewportSize(renderer::Viewport& viewport, ImTextureID& textureId,
                          std::uint32_t width, std::uint32_t height,
                          std::uint32_t displayTargetIndex) const;
};

};  // namespace vkit
