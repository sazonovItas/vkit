#pragma once

#include <array>
#include <memory>
#include <vector>
#include <vk_mem_alloc.hpp>

#include "vkit/asset/asset.hpp"
#include "vkit/compute/async_compute.hpp"
#include "vkit/compute/heightmap_dispatcher.hpp"
#include "vkit/compute/noise_dispatcher.hpp"
#include "vkit/compute/sobel_dispatcher.hpp"
#include "vkit/compute/tint_dispatcher.hpp"
#include "vkit/controller/camera.hpp"
#include "vkit/controller/workflow.hpp"
#include "vkit/graphics/descriptor_buffer.hpp"
#include "vkit/graphics/device.hpp"
#include "vkit/graphics/instance.hpp"
#include "vkit/graphics/surface.hpp"
#include "vkit/graphics/swapchain.hpp"
#include "vkit/imgui/imgui_renderer.hpp"
#include "vkit/imgui/imgui_window_manager.hpp"
#include "vkit/imgui/window_imgui_host.hpp"
#include "vkit/imgui/windows/ge/graph_editor.hpp"
#include "vkit/imgui/windows/ge/graph_node_inspector.hpp"
#include "vkit/imgui/windows/viewer.hpp"
#include "vkit/platform/file_dialog.hpp"
#include "vkit/renderer/descriptor_set_layout/primitive.hpp"
#include "vkit/renderer/descriptor_set_layout/scene.hpp"
#include "vkit/renderer/pipeline_layout/primitive_debug.hpp"
#include "vkit/renderer/pipeline_layout/ray_sphere_debug.hpp"
#include "vkit/renderer/renderer.hpp"
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
  void initWorkflow();
  void initImgui();
  void initViewports();
  void initDebugRendering();

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
  } sys_;

  struct EngineContext {
    std::shared_ptr<texture::TextureManager> textureManager;
    std::shared_ptr<texture::TextureUploader> textureUploader;

    std::shared_ptr<workflow::ExecutionContext> executionContext;

    std::shared_ptr<compute::AsyncCompute> asyncCompute;
    std::shared_ptr<compute::NoiseDispatcher> noiseDispatcher;
    std::shared_ptr<compute::SobelDispatcher> sobelDispatcher;
    std::shared_ptr<compute::HeightMapDispatcher> heightMapDispatcher;
    std::shared_ptr<compute::TintDispatcher> tintDispatcher;

    std::unique_ptr<workflow::Workflow> workflow;
    std::unique_ptr<controller::WorkflowController> workflowController;

    void update() {
      workflow->execute();

      executionContext->update();
      textureUploader->update();
      noiseDispatcher->update();
      sobelDispatcher->update();
      heightMapDispatcher->update();
      tintDispatcher->update();
      executionContext->update();
    }
  } engine_;

  struct UIContext {
    std::unique_ptr<imgui::ImguiRenderer> renderer;
    std::unique_ptr<imgui::WindowImguiHost> host;
    std::unique_ptr<imgui::ImguiWindowManager> windowManager;

    std::shared_ptr<imgui::windows::Viewer> sceneViewer;
    std::shared_ptr<imgui::windows::Viewer> materialViewer;
    std::shared_ptr<imgui::windows::ge::GraphEditorWindow> graphWindow;
    std::shared_ptr<imgui::windows::ge::GraphNodeInspectorWindow>
        graphInspector;
  } ui_;

  struct SceneContext {
    std::shared_ptr<scene::Camera> sceneCamera;
    std::shared_ptr<scene::Camera> materialCamera;
    controller::FreeCameraController sceneController;
    controller::OrbitalCameraController materialController;
  } scene_;

  struct DebugRenderContext {
    std::shared_ptr<asset::Asset> loadedAsset;
    std::unique_ptr<renderer::dsl::SceneSetLayout> sceneSetLayout;
    std::unique_ptr<renderer::dsl::PrimitiveSetLayout> primitiveSetLayout;

    std::unique_ptr<renderer::pl::RaySphereDebugPipelineLayout> raySphereLayout;
    vk::UniquePipeline raySpherePipeline;

    std::unique_ptr<renderer::pl::PrimitiveMaterialPipelineLayout>
        primitiveLayout;
    vk::UniquePipeline primitivePipeline;

    vk::UniqueDescriptorPool descriptorPool;
  } debug_;

  struct Frame {
    renderer::Viewport sceneViewport;
    ImTextureID sceneTextureId{0};

    renderer::Viewport materialViewport;
    ImTextureID materialTextureId{0};

    vk::DescriptorSet sceneDescriptorSet;
    vk::DescriptorSet materialDescriptorSet;
    vk::DescriptorSet primitiveDescriptorSet;

    std::unique_ptr<graphics::DescriptorBuffer> sceneCameraBuffer;
    std::unique_ptr<graphics::DescriptorBuffer> materialCameraBuffer;
    std::unique_ptr<graphics::DescriptorBuffer> primitiveSSBO;
    std::unique_ptr<graphics::DescriptorBuffer> jointSSBO;
  };

  std::array<Frame, kMaxFramesInFlight> frames_;

  void ensureViewportSize(renderer::Viewport& viewport, ImTextureID& textureId,
                          std::uint32_t width, std::uint32_t height,
                          std::uint32_t displayTargetIndex) const;
};

};  // namespace vkit
