#pragma once

#include <filesystem>
#include <optional>

#include "bindless_set_manager.hpp"
#include "dear_imgui.hpp"
#include "descriptor_buffer.hpp"
#include "fastgltf/types.hpp"
#include "gltf/asset.hpp"
#include "resource_buffering.hpp"
#include "shader_program.hpp"
#include "swapchain.hpp"
#include "transform.hpp"
#include "vk_mem_alloc.hpp"
#include "vku/scoped/device_waiter.hpp"
#include "vulkan/descriptor_set_layout/material.hpp"
#include "vulkan/descriptor_set_layout/scene.hpp"
#include "vulkan/gpu.hpp"
#include "vulkan/pipeline_layout/pbr.hpp"
#include "vulkan/vulkan.hpp"
#include "window.hpp"

namespace lvk {
class App {
 public:
  void run();

 private:
  struct Camera {
    glm::vec3 position{1.0F, 1.0F, 1.0F};
    glm::vec3 target{0.0F};
    glm::vec3 up{0.0F, 1.0F, 0.0F};
  };

  struct UBO {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 projection;
    alignas(16) glm::vec3 cameraPosition;
  };

  struct UBOParams {
    glm::vec3 lightDir{0.0, -1.0, -1.0};
  };

  struct alignas(16) Material {
    glm::vec4 baseColorFactor;
    glm::vec4 emissiveFactor;

    float metallicFactor;
    float roughnessFactor;
    float alphaMaskCutoff;
    float emissiveStrength;

    int32_t baseColorTextureIdx;
    int32_t metallicRoughnessTextureIdx;
    int32_t normalTextureIdx;
    int32_t occlusionTextureIdx;

    int32_t emissiveTextureIdx;
    int32_t pad0;
    int32_t pad1;
    int32_t pad2;
  };

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
  [[nodiscard]] auto allocateSets() const -> std::vector<vk::DescriptorSet>;

  void update();
  void updateMaterials();

  void updateDescriptorSets() const;

  void draw(vk::CommandBuffer cb) const;
  void bindDescriptorSets(vk::CommandBuffer cb) const;

  void drawNode(vk::CommandBuffer cb, const fastgltf::Node& node,
                const fastgltf::math::fmat4x4& transform,
                bool isTransparentPass) const;

  void inspect();

  void loadGLTF(const std::filesystem::path& path);

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

  void createGraphicsCommandPool();

  void createImgui();

  std::filesystem::path assetDir_;

  Camera camera_;
  Transform transform_;

  glfw::Window window_;
  glm::ivec2 frameBufferSize_;

  vk::UniqueInstance instance_;
  vk::UniqueDebugUtilsMessengerEXT debugMessanger_;
  vk::UniqueSurfaceKHR surface_;

  std::optional<vkit::vulkan::Gpu> gpu_;

  std::optional<Swapchain> swapchain_;

  vk::UniqueCommandPool renderCommandPool_;
  vk::UniqueCommandPool graphicsCommandPool_;

  std::size_t frameIndex_{};
  Buffered<RenderSync> renderSync_{};

  std::optional<DearImGui> imgui_;

  vk::UniqueDescriptorPool descriptorPool_;

  std::optional<vkit::vulkan::dsl::SceneLayout> sceneSetLayout_;
  std::optional<vkit::vulkan::dsl::MaterialLayout> materialSetLayout_;
  std::optional<vkit::vulkan::dsl::BindlessLayout> bindlessSetLayout_;

  std::optional<vkit::vulkan::pl::PBRPipelineLayout> pbrPipelineLayout_;

  std::optional<ShaderProgram> shader_;

  UBO ubo_;
  UBOParams uboParams_;
  std::vector<Material> materials_;

  std::optional<DescriptorBuffer<kResourceBufferingV>> uboBuffers_;
  std::optional<DescriptorBuffer<kResourceBufferingV>> uboParamsBuffers_;
  std::optional<DescriptorBuffer<kResourceBufferingV>> materialsBuffers_;

  Buffered<vk::UniqueDescriptorSet> sceneSets_;
  Buffered<vk::UniqueDescriptorSet> materialsSets_;
  std::optional<BindlessSetManager> bindlessSetManager_;

  std::optional<RenderTarget> renderTarget_;

  std::optional<gltf::Asset> gltfAsset_;

  vku::DeviceWaiter deviceWaiter_;
};
}  // namespace lvk
