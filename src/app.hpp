#pragma once

#include <array>
#include <memory>
#include <vector>
#include <vk_mem_alloc.hpp>

#include "vkit/asset/asset.hpp"
#include "vkit/compute/async_compute.hpp"
#include "vkit/controller/camera.hpp"
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
#include "vkit/renderer/descriptor_set_layout/primitive.hpp"
#include "vkit/renderer/descriptor_set_layout/scene.hpp"
#include "vkit/renderer/pipeline_layout/primitive_debug.hpp"
#include "vkit/renderer/pipeline_layout/ray_sphere_debug.hpp"
#include "vkit/renderer/renderer.hpp"
#include "vkit/renderer/viewport.hpp"
#include "vkit/scene/camera.hpp"
#include "vkit/texture/compute_dispatcher.hpp"
#include "vkit/texture/manager.hpp"
#include "vkit/texture/texture_uploader.hpp"
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

  std::vector<vk::UniqueSemaphore> imageAvailableSemaphores_;

  std::unique_ptr<imgui::ImguiRenderer> imguiRenderer_;
  std::unique_ptr<imgui::WindowImguiHost> imguiHost_;
  std::unique_ptr<imgui::ImguiWindowManager> windowManager_;

  std::shared_ptr<texture::TextureManager> textureManager_;
  std::shared_ptr<workflow::ExecutionContext> executionContext_;
  std::shared_ptr<texture::TextureUploader> textureUploader_;
  std::shared_ptr<compute::AsyncCompute> asyncCompute_;
  std::shared_ptr<texture::ComputeDispatcher> computeDispatcher_;
  std::unique_ptr<workflow::Workflow> workflow_;

  std::shared_ptr<imgui::windows::Viewer> sceneViewer_;
  std::shared_ptr<imgui::windows::Viewer> materialViewer_;
  std::shared_ptr<imgui::windows::ge::GraphEditorWindow> graphWindow_;
  std::shared_ptr<imgui::windows::ge::GraphNodeInspectorWindow>
      graphNodeInspectorWindow_;

  std::shared_ptr<scene::Camera> sceneCamera_;
  std::shared_ptr<scene::Camera> materialCamera_;
  controller::FreeCameraController sceneController_;
  controller::OrbitalCameraController materialController_;

  std::unique_ptr<renderer::dsl::SceneSetLayout> sceneSetLayout_;
  std::unique_ptr<renderer::pl::RaySphereDebugPipelineLayout>
      raySpherePipelineLayout_;
  vk::UniquePipeline raySpherePipeline_;

  std::shared_ptr<asset::Asset> loadedAsset_;
  std::unique_ptr<renderer::dsl::PrimitiveSetLayout> primitiveSetLayout_;
  std::unique_ptr<renderer::pl::PrimitiveMaterialPipelineLayout>
      primitivePipelineLayout_;
  vk::UniquePipeline primitivePipeline_;

  vk::UniqueDescriptorPool descriptorPool_;

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
                          std::uint32_t displayTargetIndex);
};

};  // namespace vkit
