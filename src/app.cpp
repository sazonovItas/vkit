#include "app.hpp"

#include <backends/imgui_impl_glfw.h>
#include <imgui.h>

#include <algorithm>
#include <array>
#include <cassert>
#include <chrono>
#include <cstddef>
#include <optional>
#include <print>
#include <ranges>
#include <stdexcept>
#include <vector>

#include "GLFW/glfw3.h"
#include "fastgltf/tools.hpp"
#include "fastgltf/types.hpp"
#include "glm/ext/matrix_clip_space.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/fwd.hpp"
#include "resource_buffering.hpp"
#include "shader_program.hpp"
#include "stb_image.h"
#include "vku/utils/utils.hpp"
#include "vulkan/descriptor_set_layout/scene.hpp"
#include "vulkan/gpu.hpp"
#include "vulkan/pipeline_layout/pbr.hpp"
#include "vulkan/vulkan.hpp"
#include "window.hpp"

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE;

#define MODEL_PATH "models/cannon/scene.gltf"

namespace {
constexpr auto kVkVersionV = VK_MAKE_VERSION(1, 3, 0);

vk::Bool32 VKAPI_CALL
debugCallback(vk::DebugUtilsMessageSeverityFlagBitsEXT /*messageSeverity*/,
              vk::DebugUtilsMessageTypeFlagsEXT /*messageType*/,
              const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData,
              void* /*pUserData*/) {
  if (pCallbackData && pCallbackData->pMessage) {
    std::println("[Validation]: {}", pCallbackData->pMessage);
  }

  return vk::False;
}

[[nodiscard]] auto locateAssetsDir() -> std::filesystem::path {
  static constexpr std::string_view kDirNameV{"assets"};

  for (auto path = std::filesystem::current_path();
       !path.empty() && path.has_parent_path(); path = path.parent_path()) {
    auto ret = path / kDirNameV;
    if (std::filesystem::is_directory(ret)) {
      return ret;
    }
  }

  std::println("[vkit] warning could not locate '{}' directory", kDirNameV);
  return std::filesystem::current_path();
}

[[nodiscard]] auto getLayers(std::span<char const* const> desired)
    -> std::vector<char const*> {
  auto ret = std::vector<char const*>{};
  ret.reserve(desired.size());

  auto const available = vk::enumerateInstanceLayerProperties();
  for (char const* layer : desired) {
    auto const pred = [layer = std::string_view{layer}](
                          vk::LayerProperties const& properties) {
      return properties.layerName == layer;
    };

    if (std::ranges::find_if(available, pred) == available.end()) {
      std::println("[lvk] [WARNING] Vulkan layer '{}' not found", layer);
      continue;
    }

    ret.push_back(layer);
  }

  return ret;
}

template <typename T>
[[nodiscard]] constexpr auto toByteArray(T const& t) {
  return std::bit_cast<std::array<std::byte, sizeof(T)>>(t);
}

template <typename T>
[[nodiscard]] constexpr auto toByteSpan(std::vector<T> const& v) {
  return std::as_bytes(std::span{v});
}

constexpr auto layoutBinding(std::uint32_t binding,
                             vk::DescriptorType const type) {
  return vk::DescriptorSetLayoutBinding{
      binding,
      type,
      1,
      vk::ShaderStageFlagBits::eAllGraphics,
  };
}

struct BitmapData {
  std::vector<std::byte> storage;
  vku::Bitmap bitmap;
};

float hash(int x, int y) {
  std::hash<int> h;
  return static_cast<float>((h((x * 73856093) ^ (y * 19349663))) & 0xffff) /
         65535.0F;
}

auto generateBricks(int width, int height, float brickWidth, float brickHeight,
                    float mortarThickness) -> std::vector<std::byte> {
  std::vector<std::byte> result;
  result.resize(width * height * 4);

  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      float fx = static_cast<float>(x) / width;
      float fy = static_cast<float>(y) / height;

      float bx = fx * (width / brickWidth);
      float by = fy * (height / brickHeight);

      int row = static_cast<int>(std::floor(by));

      if (row % 2 == 1) bx += 0.5F;

      float brick_local_x = bx - std::floor(bx);
      float brick_local_y = by - std::floor(by);

      float local_x = std::fmod(static_cast<float>(x), brickWidth);
      float local_y = std::fmod(static_cast<float>(y), brickHeight);

      if (row % 2 == 1)
        local_x =
            std::fmod(static_cast<float>(x) + (brickWidth * 0.5F), brickWidth);

      bool is_mortar = local_x < mortarThickness || local_y < mortarThickness;

      int index = (y * width + x) * 4;

      if (is_mortar) {
        uint8_t mortar = 200;
        result[index + 0] = std::byte(mortar);
        result[index + 1] = std::byte(mortar);
        result[index + 2] = std::byte(mortar);
        result[index + 3] = std::byte(255);
      } else {
        float noise = hash(static_cast<int>(std::floor(bx)),
                           static_cast<int>(std::floor(by)));
        float variation = 0.8F + (noise * 0.4F);

        auto r = static_cast<uint8_t>(150 * variation);
        auto g = static_cast<uint8_t>(50 * variation);
        auto b = static_cast<uint8_t>(40 * variation);

        result[index + 0] = std::byte(r);
        result[index + 1] = std::byte(g);
        result[index + 2] = std::byte(b);
        result[index + 3] = std::byte(255);
      }
    }
  }

  return result;
}

auto generateStoneTiles(int width, int height, int tileSize,
                        float mortarThickness) -> std::vector<std::byte> {
  std::vector<std::byte> result;
  result.resize(width * height * 4);

  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      int tile_x = x / tileSize;
      int tile_y = y / tileSize;

      auto local_x = static_cast<float>(x % tileSize);
      auto local_y = static_cast<float>(y % tileSize);

      bool is_mortar = local_x < mortarThickness || local_y < mortarThickness;

      int index = (y * width + x) * 4;

      if (is_mortar) {
        uint8_t mortar = 180;
        result[index + 0] = std::byte(mortar);
        result[index + 1] = std::byte(mortar);
        result[index + 2] = std::byte(mortar);
        result[index + 3] = std::byte(255);
      } else {
        float base_noise = hash(tile_x, tile_y);
        float variation = 0.7F + (base_noise * 0.6F);

        float fine_noise = hash(x, y);
        float grain = 0.9F + (fine_noise * 0.2F);

        auto r = static_cast<uint8_t>(120 * variation * grain);
        auto g = static_cast<uint8_t>(110 * variation * grain);
        auto b = static_cast<uint8_t>(100 * variation * grain);

        result[index + 0] = std::byte(r);
        result[index + 1] = std::byte(g);
        result[index + 2] = std::byte(b);
        result[index + 3] = std::byte(255);
      }
    }
  }

  return result;
}
};  // namespace

namespace lvk {
void App::run() {
  assetDir_ = locateAssetsDir();

  createWindow();
  createInstance();
  createSurface();
  createDevice();
  createSwapchain();

  createRenderCommandPool();
  createRenderSync();

  createDescriptorLayouts();
  createPipelineLayouts();
  createShaders();

  createDescriptorResources();
  createDescriptorPool();
  createDescriptorSets();
  createBindlessSetManager();

  createGraphicsCommandPool();

  createImgui();

  loadGLTF(assetPath(MODEL_PATH));

  mainLoop();
}

void App::mainLoop() {
  while (glfwWindowShouldClose(window_.get()) == GLFW_FALSE) {
    glfwPollEvents();

    if (!acquireRenderTarget()) {
      continue;
    }

    const auto cb = beginFrame();

    transitionForRender(cb);
    render(cb);

    transitionForPresent(cb);
    submitAndPresent();
  }
}

auto App::assetPath(std::string_view uri) const -> std::filesystem::path {
  return assetDir_ / uri;
}

void App::loadGLTF(const std::filesystem::path& path) {
  const auto copy_info = vku::DeviceCopyInfo{
      .device = *gpu_->device,
      .commandPool = *graphicsCommandPool_,
      .queue = gpu_->queues.graphicsPresent,
  };

  gltfAsset_.emplace(path, gpu_->allocator, copy_info);

  for (const auto& [idx, texture] : gltfAsset_->textures) {
    bindlessSetManager_->addTexture2D(*gpu_->device, idx, *texture.get());
  }

  materials_.reserve(gltfAsset_->materials.size());
  for (const auto& [idx, m] : gltfAsset_->materials) {
    auto material = Material{
        .baseColorFactor = m.baseColorFactor,
        .emissiveFactor = m.emissiveFactor,
        .baseColorTextureIdx =
            m.baseColorTexture
                ? static_cast<int32_t>(m.baseColorTexture.value())
                : -1,
        .metallicRoughnessTextureIdx =
            m.metallicRoughnessTexture
                ? static_cast<int32_t>(m.baseColorTexture.value())
                : -1,
        .normalTextureIdx =
            m.normalTexture ? static_cast<int32_t>(m.baseColorTexture.value())
                            : -1,
        .occlusionTextureIdx =
            m.occlusionTexture
                ? static_cast<int32_t>(m.baseColorTexture.value())
                : -1,
        .metallicFactor = m.metallicFactor,
        .roughnessFactor = m.roughnessFactor,
    };

    materials_.emplace_back(material);
  }

  updateMaterials();
}

void App::draw(vk::CommandBuffer cb) const {
  shader_->bind(cb, frameBufferSize_);
  bindDescriptorSets(cb);

  fastgltf::iterateSceneNodes(
      gltfAsset_->asset, gltfAsset_->sceneIdx, fastgltf::math::fmat4x4(),
      [&](const fastgltf::Node& node,
          const fastgltf::math::fmat4x4& /*transform*/) {
        drawNode(cb, node, fastgltf::AlphaMode::Opaque);
      });
}

void App::bindDescriptorSets(vk::CommandBuffer cb) const {
  const auto descriptor_sets = std::array<vk::DescriptorSet, 3>{
      *sceneSets_.at(frameIndex_),
      *materialsSets_.at(frameIndex_),
      bindlessSetManager_->getSet(),
  };

  cb.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
                        pbrPipelineLayout_->get(), 0, descriptor_sets, {});
}

void App::drawNode(vk::CommandBuffer cb, const fastgltf::Node& node,
                   fastgltf::AlphaMode /*alphaMode*/) const {
  using PbrPushConstants = vkit::vulkan::pl::PBRPipelineLayout::PushConstants;

  if (node.meshIndex) {
    auto it = gltfAsset_->meshes.find(node.meshIndex.value());
    assert(it != gltfAsset_->meshes.end());

    const auto& mesh = it->second;

    auto push_constants = PbrPushConstants{
        .meshIdx = static_cast<uint32_t>(node.meshIndex.value()),
    };
    for (const auto& primitive : mesh->primitives) {
      push_constants.materialIdx = primitive.materialIdx;
      push_constants.vertices =
          primitive.vertexBuffer.getAddress(*gpu_->device);

      cb.pushConstants(
          pbrPipelineLayout_->get(),
          vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
          0, sizeof(PbrPushConstants), &push_constants);

      cb.bindIndexBuffer(primitive.indexBuffer.buffer, 0,
                         vk::IndexType::eUint32);

      cb.drawIndexed(primitive.indexCount, 1, 0, 0, 0);
    }
  }
}

void App::update() {
  auto const half_size = 0.5F * glm::vec2{frameBufferSize_};

  auto model = transform_.modelMatrix();
  auto view = glm::lookAt(camera_.position, camera_.target, camera_.up);
  auto projection = glm::perspective(glm::radians(90.0F),
                                     static_cast<float>(frameBufferSize_.x) /
                                         static_cast<float>(frameBufferSize_.y),
                                     0.1F, 1000.0F);

  ubo_ = UBO{
      .model = model,
      .view = view,
      .projection = projection,
      .cameraPosition = camera_.position,
  };

  const auto ubo_bytes =
      std::bit_cast<std::array<std::byte, sizeof(ubo_)>>(ubo_);

  uboBuffers_->writeAt(frameIndex_, ubo_bytes);

  const auto ubo_params_bytes =
      std::bit_cast<std::array<std::byte, sizeof(uboParams_)>>(uboParams_);
  uboParamsBuffers_->writeAt(frameIndex_, ubo_params_bytes);
}

void App::updateMaterials() {
  materialsBuffer_->writeAt(0, toByteSpan(materials_));
}

void App::updateDescriptorSets() const {
  auto writes = std::array<vk::WriteDescriptorSet, 3>{};

  auto write = vk::WriteDescriptorSet{};

  const auto scene_set = *sceneSets_.at(frameIndex_);
  const auto ubo_info = uboBuffers_->descriptorInfoAt(frameIndex_);
  write.setBufferInfo(ubo_info)
      .setDescriptorType(vk::DescriptorType::eUniformBuffer)
      .setDescriptorCount(1)
      .setDstSet(scene_set)
      .setDstBinding(vkit::vulkan::dsl::SceneLayout::kUBOBindingIdx);
  writes[0] = write;

  const auto ubo_params_info = uboParamsBuffers_->descriptorInfoAt(frameIndex_);
  write.setBufferInfo(ubo_params_info)
      .setDescriptorType(vk::DescriptorType::eUniformBuffer)
      .setDescriptorCount(1)
      .setDstSet(scene_set)
      .setDstBinding(vkit::vulkan::dsl::SceneLayout::kUBOParamsBindingIdx);
  writes[1] = write;

  const auto materials_set = *materialsSets_.at(frameIndex_);
  const auto materials_info = materialsBuffer_->descriptorInfoAt(0);
  write.setBufferInfo(materials_info)
      .setDescriptorType(vk::DescriptorType::eStorageBuffer)
      .setDescriptorCount(1)
      .setDstSet(materials_set)
      .setDstBinding(0);
  writes[2] = write;

  gpu_->device->updateDescriptorSets(writes, {});
}

void App::inspect() {
  ImGui::SetNextWindowSize({300.0F, 320.0F}, ImGuiCond_Once);
  if (ImGui::Begin("Inspect")) {
    static auto const kCameraView = [](Camera& out) {
      ImGui::DragFloat3("position", &out.position.x);
      ImGui::DragFloat3("target", &out.target.x);
      ImGui::DragFloat3("up", &out.up.x);
    };

    ImGui::Separator();
    if (ImGui::TreeNode("Camera")) {
      kCameraView(camera_);
      ImGui::TreePop();
    }

    static auto const kInspectTransform = [](Transform& out) {
      ImGui::DragFloat3("position", &out.position.x);
      ImGui::DragFloat3("rotation", &out.rotation.x);
      ImGui::DragFloat3("scale", &out.scale.x, 0.1F);
    };

    ImGui::Separator();
    if (ImGui::TreeNode("Transform")) {
      kInspectTransform(transform_);
      ImGui::TreePop();
    }

    static auto const kInspectUBOParams = [](UBOParams& out) {
      ImGui::DragFloat3("lightDir", &out.lightDir.x);
    };

    ImGui::Separator();
    if (ImGui::TreeNode("Params")) {
      kInspectUBOParams(uboParams_);
      ImGui::TreePop();
    }
  }
  ImGui::End();
}

auto App::acquireRenderTarget() -> bool {
  frameBufferSize_ = glfw::framebufferSize(window_.get());
  if (frameBufferSize_.x <= 0 || frameBufferSize_.y <= 0) {
    return false;
  }

  auto& render_sync = renderSync_.at(frameIndex_);

  static constexpr auto kFenceTimeoutV = static_cast<std::uint64_t>(
      std::chrono::nanoseconds{std::chrono::seconds{3}}.count());
  auto result =
      gpu_->device->waitForFences(*render_sync.drawn, vk::True, kFenceTimeoutV);
  if (result != vk::Result::eSuccess) {
    throw std::runtime_error{"failed to wait for render fence"};
  }

  renderTarget_ = swapchain_->acquireNextImage(*render_sync.draw);
  if (!renderTarget_) {
    swapchain_->recreate(frameBufferSize_);
    return false;
  }

  gpu_->device->resetFences(*render_sync.drawn);

  imgui_->newFrame();

  return true;
}

auto App::beginFrame() -> vk::CommandBuffer {
  auto const& render_sync = renderSync_.at(frameIndex_);

  auto bi = vk::CommandBufferBeginInfo{};
  bi.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
  render_sync.cb.begin(bi);

  return render_sync.cb;
}

void App::transitionForRender(vk::CommandBuffer cb) const {
  auto dependency_info = vk::DependencyInfo{};

  auto color_barrier = swapchain_->baseColorBarrier();
  color_barrier.setOldLayout(vk::ImageLayout::eUndefined)
      .setNewLayout(vk::ImageLayout::eColorAttachmentOptimal)
      .setSrcAccessMask(vk::AccessFlagBits2::eColorAttachmentRead |
                        vk::AccessFlagBits2::eColorAttachmentWrite)
      .setSrcStageMask(vk::PipelineStageFlagBits2::eColorAttachmentOutput)
      .setDstAccessMask(color_barrier.srcAccessMask)
      .setDstStageMask(color_barrier.srcStageMask);

  auto depth_barrier = swapchain_->baseDepthBarrier();
  depth_barrier.setOldLayout(vk::ImageLayout::eUndefined)
      .setNewLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal)
      .setSrcAccessMask(vk::AccessFlagBits2::eNone)
      .setSrcStageMask(vk::PipelineStageFlagBits2::eTopOfPipe)
      .setDstAccessMask(vk::AccessFlagBits2::eDepthStencilAttachmentRead |
                        vk::AccessFlagBits2::eDepthStencilAttachmentWrite)
      .setDstStageMask(vk::PipelineStageFlagBits2::eEarlyFragmentTests |
                       vk::PipelineStageFlagBits2::eLateFragmentTests);

  auto swapchain_color_barrier = swapchain_->baseBarrier();
  swapchain_color_barrier.setOldLayout(vk::ImageLayout::eUndefined)
      .setNewLayout(vk::ImageLayout::eColorAttachmentOptimal)
      .setSrcAccessMask(vk::AccessFlagBits2::eColorAttachmentRead |
                        vk::AccessFlagBits2::eColorAttachmentWrite)
      .setSrcStageMask(vk::PipelineStageFlagBits2::eColorAttachmentOutput)
      .setDstAccessMask(swapchain_color_barrier.srcAccessMask)
      .setDstStageMask(swapchain_color_barrier.srcStageMask);

  auto barriers =
      std::array{color_barrier, depth_barrier, swapchain_color_barrier};
  dependency_info.setImageMemoryBarriers(barriers);
  cb.pipelineBarrier2(dependency_info);
}

void App::render(vk::CommandBuffer cb) {
  auto color_attachment = vk::RenderingAttachmentInfo{};
  color_attachment.setImageView(renderTarget_->colorImageView)
      .setImageLayout(vk::ImageLayout::eColorAttachmentOptimal)
      .setResolveMode(vk::ResolveModeFlagBits::eAverage)
      .setResolveImageView(renderTarget_->swapchainImageView)
      .setResolveImageLayout(vk::ImageLayout::eColorAttachmentOptimal)
      .setLoadOp(vk::AttachmentLoadOp::eClear)
      .setStoreOp(vk::AttachmentStoreOp::eDontCare)
      .setClearValue(vk::ClearColorValue{0.0F, 0.0F, 0.0F, 1.0F});

  auto depth_attachment = vk::RenderingAttachmentInfo{};
  depth_attachment.setImageView(renderTarget_->depthImageView)
      .setImageLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal)
      .setLoadOp(vk::AttachmentLoadOp::eClear)
      .setStoreOp(vk::AttachmentStoreOp::eDontCare)
      .setClearValue(vk::ClearDepthStencilValue{1.0F, 0});

  auto rendering_info = vk::RenderingInfo{};
  auto const render_area = vk::Rect2D{vk::Offset2D{}, renderTarget_->extent};
  rendering_info.setRenderArea(render_area)
      .setLayerCount(1)
      .setColorAttachmentCount(1)
      .setPColorAttachments(&color_attachment)
      .setPDepthAttachment(&depth_attachment);

  update();
  updateDescriptorSets();

  cb.beginRendering(rendering_info);
  draw(cb);
  cb.endRendering();

  auto imgui_attachment = vk::RenderingAttachmentInfo{};
  imgui_attachment.setImageView(renderTarget_->swapchainImageView)
      .setLoadOp(vk::AttachmentLoadOp::eLoad)
      .setStoreOp(vk::AttachmentStoreOp::eStore);

  auto imgui_rendering = vk::RenderingInfo{};
  imgui_rendering.setRenderArea(render_area)
      .setLayerCount(1)
      .setColorAttachmentCount(1)
      .setPColorAttachments(&imgui_attachment);

  inspect();
  imgui_->endFrame();

  cb.beginRendering(imgui_rendering);
  imgui_->render(cb);
  cb.endRendering();
}

void App::transitionForPresent(vk::CommandBuffer cb) const {
  auto barrier = swapchain_->baseBarrier();
  barrier.setOldLayout(vk::ImageLayout::eAttachmentOptimal)
      .setNewLayout(vk::ImageLayout::ePresentSrcKHR)
      .setSrcAccessMask(vk::AccessFlagBits2::eColorAttachmentRead |
                        vk::AccessFlagBits2::eColorAttachmentWrite)
      .setSrcStageMask(vk::PipelineStageFlagBits2::eColorAttachmentOutput)
      .setDstAccessMask(barrier.srcAccessMask)
      .setDstStageMask(barrier.srcStageMask);

  auto dependency_info = vk::DependencyInfo{};
  dependency_info.setImageMemoryBarriers(barrier);

  cb.pipelineBarrier2(dependency_info);
}

void App::submitAndPresent() {
  auto const& render_sync = renderSync_.at(frameIndex_);
  render_sync.cb.end();

  auto submit_info = vk::SubmitInfo2{};
  auto const cb_info = vk::CommandBufferSubmitInfo{render_sync.cb};

  auto wait_semaphore_info = vk::SemaphoreSubmitInfo{};
  wait_semaphore_info.setSemaphore(*render_sync.draw)
      .setStageMask(vk::PipelineStageFlagBits2::eColorAttachmentOutput);

  auto signal_semaphore_info = vk::SemaphoreSubmitInfo{};
  signal_semaphore_info.setSemaphore(swapchain_->getPresentSemaphore())
      .setStageMask(vk::PipelineStageFlagBits2::eColorAttachmentOutput);

  submit_info.setCommandBufferInfos(cb_info)
      .setWaitSemaphoreInfos(wait_semaphore_info)
      .setSignalSemaphoreInfos(signal_semaphore_info);

  gpu_->queues.graphicsPresent.submit2(submit_info, *render_sync.drawn);

  frameIndex_ = (frameIndex_ + 1) % renderSync_.size();
  renderTarget_.reset();

  auto const fb_size_changed = frameBufferSize_ != swapchain_->getSize();
  auto const out_of_date = !swapchain_->present(gpu_->queues.graphicsPresent);
  if (fb_size_changed || out_of_date) swapchain_->recreate(frameBufferSize_);
}

void App::createDescriptorLayouts() {
  sceneSetLayout_.emplace(*gpu_->device);
  materialSetLayout_.emplace(*gpu_->device);
  bindlessSetLayout_.emplace(*gpu_->device);
}

void App::createPipelineLayouts() {
  pbrPipelineLayout_.emplace(
      *gpu_->device,
      std::tie(*sceneSetLayout_, *materialSetLayout_, *bindlessSetLayout_));
}

void App::createShaders() {
  {
    const auto vertex_shader_spirv =
        vku::toSpirV(assetPath("shaders/primitive.vert"));
    const auto fragment_shader_spirv =
        vku::toSpirV(assetPath("shaders/primitive.frag"));

    auto set_layouts = std::array<vk::DescriptorSetLayout, 3>{
        sceneSetLayout_->get(),
        materialSetLayout_->get(),
        bindlessSetLayout_->get(),
    };

    auto push_constant_ranges = std::array<vk::PushConstantRange, 1>{
        vkit::vulkan::pl::PBRPipelineLayout::kPushConstantRange,
    };

    const auto ci = ShaderProgramCreateInfo{
        .device = *gpu_->device,
        .vertexSpirv = vertex_shader_spirv,
        .fragmentSpirv = fragment_shader_spirv,
        .setLayouts = set_layouts,
        .pushConstantRanges = push_constant_ranges,
    };
    shader_.emplace(ci);
  }
}

void App::createDescriptorResources() {
  uboBuffers_.emplace(gpu_->allocator, gpu_->queueFamilies.transfer,
                      vk::BufferUsageFlagBits::eUniformBuffer);
  uboParamsBuffers_.emplace(gpu_->allocator, gpu_->queueFamilies.transfer,
                            vk::BufferUsageFlagBits::eUniformBuffer);
  materialsBuffer_.emplace(gpu_->allocator, gpu_->queueFamilies.transfer,
                           vk::BufferUsageFlagBits::eStorageBuffer);
}

void App::createDescriptorPool() {
  static constexpr auto kPoolSizeV = std::array{
      vk::DescriptorPoolSize{
          vk::DescriptorType::eUniformBuffer,
          2 * kResourceBufferingV,
      },
      vk::DescriptorPoolSize{
          vk::DescriptorType::eStorageBuffer,
          kResourceBufferingV,
      },
  };

  auto pool_ci = vk::DescriptorPoolCreateInfo{};
  pool_ci.setPoolSizes(kPoolSizeV).setMaxSets(16);
  descriptorPool_ = gpu_->device->createDescriptorPoolUnique(pool_ci);
}

void App::createDescriptorSets() {
  {
    auto layouts = std::array<vk::DescriptorSetLayout, kResourceBufferingV>{};
    layouts.fill(sceneSetLayout_->get());

    auto alloc_info = vk::DescriptorSetAllocateInfo{};
    alloc_info.setDescriptorPool(*descriptorPool_).setSetLayouts(layouts);

    auto descriptor_sets =
        gpu_->device->allocateDescriptorSetsUnique(alloc_info);
    assert(descriptor_sets.size() == kResourceBufferingV);

    for (auto&& [idx, set] : std::ranges::views::enumerate(descriptor_sets)) {
      sceneSets_[idx] = std::move(set);
    }
  }

  {
    auto layouts = std::array<vk::DescriptorSetLayout, kResourceBufferingV>{};
    layouts.fill(materialSetLayout_->get());

    auto alloc_info = vk::DescriptorSetAllocateInfo{};
    alloc_info.setDescriptorPool(*descriptorPool_).setSetLayouts(layouts);

    auto descriptor_sets =
        gpu_->device->allocateDescriptorSetsUnique(alloc_info);
    assert(descriptor_sets.size() == kResourceBufferingV);

    for (auto&& [idx, set] : std::ranges::views::enumerate(descriptor_sets)) {
      materialsSets_[idx] = std::move(set);
    }
  }
}

void App::createBindlessSetManager() {
  bindlessSetManager_.emplace(*gpu_, *bindlessSetLayout_);
}

void App::createGraphicsCommandPool() {
  graphicsCommandPool_ =
      gpu_->createCommandPool(gpu_->queueFamilies.graphicsPresent,
                              vk::CommandPoolCreateFlagBits::eTransient);
}

void App::createRenderCommandPool() {
  renderCommandPool_ = gpu_->createCommandPool(
      gpu_->queueFamilies.graphicsPresent,
      vk::CommandPoolCreateFlagBits::eResetCommandBuffer);
}

void App::createRenderSync() {
  auto cb_ai = vk::CommandBufferAllocateInfo{};
  cb_ai.setCommandPool(*renderCommandPool_)
      .setCommandBufferCount(static_cast<std::uint32_t>(kResourceBufferingV))
      .setLevel(vk::CommandBufferLevel::ePrimary);

  const auto cbs = gpu_->device->allocateCommandBuffers(cb_ai);
  assert(cbs.size() == renderSync_.size());

  static constexpr auto kFenceCreateInfoV =
      vk::FenceCreateInfo{vk::FenceCreateFlagBits::eSignaled};

  for (auto [sync, command_buffer] :
       std::ranges::views::zip(renderSync_, cbs)) {
    sync.cb = command_buffer;
    sync.draw = gpu_->device->createSemaphoreUnique({});
    sync.drawn = gpu_->device->createFenceUnique(kFenceCreateInfoV);
  }
}

void App::createImgui() {
  auto const imgui_ci = DearImGui::CreateInfo{
      .window = window_.get(),
      .apiVersion = kVkVersionV,
      .instance = *instance_,
      .physicalDevice = gpu_->physicalDevice,
      .queueFamily = gpu_->queueFamilies.graphicsPresent,
      .device = *gpu_->device,
      .queue = gpu_->queues.graphicsPresent,
      .colorFormat = swapchain_->getFormat(),
      .samples = vk::SampleCountFlagBits::e1,
  };

  imgui_.emplace(imgui_ci);
}

void App::createWindow() { window_ = glfw::createWindow({1280, 720}, "vkit"); }

void App::createInstance() {
  VULKAN_HPP_DEFAULT_DISPATCHER.init();

  auto const loader_version = vk::enumerateInstanceVersion();
  if (loader_version < kVkVersionV) {
    throw std::runtime_error{"loader does not support vulkan 1.3"};
  }

  auto app_info = vk::ApplicationInfo{};
  app_info.setPApplicationName("learn vulkan").setApiVersion(kVkVersionV);

  static constexpr auto kLayersV = std::array{
      "VK_LAYER_KHRONOS_validation",
  };
  auto const layers = getLayers(kLayersV);

  auto glfw_extensions = glfw::instanceExtensions();
  auto extensions =
      std::vector<const char*>(glfw_extensions.begin(), glfw_extensions.end());
  extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

  auto debug_create_info = vk::DebugUtilsMessengerCreateInfoEXT{};
  debug_create_info.setMessageSeverity(
      vk::DebugUtilsMessageSeverityFlagBitsEXT::eError |
      vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
      vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose);
  debug_create_info.setMessageType(
      vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
      vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
      vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance);
  debug_create_info.setPfnUserCallback(&debugCallback);

  auto instance_ci = vk::InstanceCreateInfo{};
  instance_ci.setPApplicationInfo(&app_info)
      .setPEnabledExtensionNames(extensions)
      .setPNext(debug_create_info);

  instance_ = vk::createInstanceUnique(instance_ci);
  VULKAN_HPP_DEFAULT_DISPATCHER.init(*instance_);

  debugMessanger_ = instance_->createDebugUtilsMessengerEXTUnique(
      debug_create_info, nullptr, VULKAN_HPP_DEFAULT_DISPATCHER);
}

void App::createSurface() {
  surface_ = glfw::createSurface(window_.get(), *instance_);
}

void App::createDevice() {
  gpu_.emplace(*instance_, *surface_);
  deviceWaiter_ = *gpu_->device;
}

void App::createSwapchain() {
  auto const size = glfw::framebufferSize(window_.get());
  swapchain_.emplace(*gpu_->device, gpu_->physicalDevice,
                     gpu_->queueFamilies.graphicsPresent, gpu_->allocator,
                     *surface_, size);
}
};  // namespace lvk
