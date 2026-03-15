#pragma once

#include <filesystem>
#include <optional>

#include "app_types.hpp"
#include "bindless_set_manager.hpp"
#include "compute_shader_program.hpp"
#include "descriptor_buffer.hpp"
#include "fastgltf/types.hpp"
#include "gltf/asset.hpp"
#include "resource_buffering.hpp"
#include "shader_program.hpp"
#include "swapchain.hpp"
#include "transform.hpp"
#include "ui.hpp"
#include "vk_mem_alloc.hpp"
#include "vku/scoped/device_waiter.hpp"
#include "vulkan/descriptor_set_layout/material.hpp"
#include "vulkan/descriptor_set_layout/scene.hpp"
#include "vulkan/gpu.hpp"
#include "vulkan/pipeline_layout/prefilter_diffuse_ibl.hpp"
#include "vulkan/pipeline_layout/prefilter_specular_ibl.hpp"
#include "vulkan/pipeline_layout/primitive.hpp"
#include "vulkan/pipeline_layout/skybox.hpp"
#include "vulkan/vulkan.hpp"
#include "window.hpp"

namespace vkit {
class App {
 public:
  void run();

 private:
  struct RenderSync {
    vk::UniqueSemaphore draw;
    vk::UniqueFence drawn;
    vk::CommandBuffer cb;
  };

  void mainLoop();

  auto acquireRenderTarget() -> bool;
  auto beginFrame() -> vk::CommandBuffer;
  void transitionForRender(vk::CommandBuffer cb) const;
  void render(vk::CommandBuffer cb);
  void transitionForPresent(vk::CommandBuffer cb) const;
  void submitAndPresent();

  [[nodiscard]] auto assetPath(std::string_view uri) const
      -> std::filesystem::path;

  void update();

  void updateDescriptorSets() const;
  void bindSkyboxDescriptorSets(vk::CommandBuffer cb) const;
  void bindPrimitiveDescriptorSets(vk::CommandBuffer cb) const;

  void drawSkybox(vk::CommandBuffer cb) const;

  void draw(vk::CommandBuffer cb) const;
  void drawNode(vk::CommandBuffer cb, const fastgltf::Node& node,
                const fastgltf::math::fmat4x4& transform,
                bool isTransparentPass) const;

  void drawUI();

  void loadGLTF(const std::filesystem::path& path);
  void loadEnvironmentMap(const std::filesystem::path& path);

  void createWindow();
  void createInstance();
  void createSurface();
  void createDevice();
  void createSwapchain();

  void createRenderCommandPool();
  void createRenderSync();

  void createDescriptorLayouts();
  void createPipelineLayouts();
  void createShaders();

  void createDescriptorResources();
  void createDescriptorPool();
  void createDescriptorSets();
  void createBindlessSetManager();

  void createCommandPools();

  void createUI();

  std::filesystem::path assetDir_;

  Camera camera_;
  Transform transform_;

  glfw::Window window_;
  glm::ivec2 frameBufferSize_;

  vk::UniqueInstance instance_;
  vk::UniqueDebugUtilsMessengerEXT debugMessanger_;
  vk::UniqueSurfaceKHR surface_;

  std::optional<vulkan::Gpu> gpu_;

  std::optional<Swapchain> swapchain_;

  vk::UniqueCommandPool renderCommandPool_;
  vk::UniqueCommandPool computeCommandPool_;
  vk::UniqueCommandPool graphicsCommandPool_;

  std::size_t frameIndex_{};
  Buffered<RenderSync> renderSync_{};

  std::optional<UI> ui_;

  vk::UniqueDescriptorPool descriptorPool_;

  std::optional<vulkan::dsl::SceneLayout> sceneSetLayout_;
  std::optional<vulkan::dsl::MaterialLayout> materialSetLayout_;
  std::optional<vulkan::dsl::BindlessLayout> bindlessSetLayout_;

  std::optional<vulkan::pl::SkyboxLayout> skyboxLayout_;
  std::optional<vulkan::pl::PrimitiveLayout> primitiveLayout_;
  std::optional<vulkan::pl::PrefilterDiffuseIBLLayout> prefilterDiffuseLayout_;
  std::optional<vulkan::pl::PrefilterSpecularIBLLayout>
      prefilterSpecularLayout_;

  std::optional<ShaderProgram> skyboxShader_;
  std::optional<ShaderProgram> primitiveShader_;
  std::optional<ComputeShaderProgram> prefilterDiffuseShader_;
  std::optional<ComputeShaderProgram> prefilterSpecularShader_;

  UBO ubo_;
  UBOParams uboParams_;
  std::vector<Light> lights_;
  std::vector<Material> materials_;

  std::optional<DescriptorBuffer<kResourceBufferingV>> uboBuffers_;
  std::optional<DescriptorBuffer<kResourceBufferingV>> uboParamsBuffers_;
  std::optional<DescriptorBuffer<kResourceBufferingV>> lightsBuffers_;
  std::optional<DescriptorBuffer<kResourceBufferingV>> materialsBuffers_;

  Buffered<vk::UniqueDescriptorSet> sceneSets_;
  Buffered<vk::UniqueDescriptorSet> materialsSets_;
  std::optional<BindlessSetManager> bindlessSetManager_;

  std::optional<RenderTarget> renderTarget_;

  std::optional<gltf::Asset> gltfAsset_;

  glm::vec4 envBaseColor_{0.2F, 0.2F, 0.2F, 1.0F};
  std::optional<std::uint32_t> currEnvMapIdx_;
  std::vector<vku::Texture2D> environmentMaps_;
  std::vector<vku::Texture2D> environmentDiffuseMaps_;
  std::vector<vku::Texture2D> environmentSpecularMaps_;

  vku::DeviceWaiter deviceWaiter_;
};
}  // namespace vkit
