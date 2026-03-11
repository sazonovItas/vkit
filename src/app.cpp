#include "app.hpp"

#include <backends/imgui_impl_glfw.h>
#include <imgui.h>

#include <algorithm>
#include <array>
#include <cassert>
#include <chrono>
#include <cstddef>
#include <cstring>
#include <optional>
#include <print>
#include <ranges>
#include <stdexcept>
#include <vector>

#include "GLFW/glfw3.h"
#include "ImGuizmo.h"
#include "fastgltf/tools.hpp"
#include "fastgltf/types.hpp"
#include "glm/ext/matrix_clip_space.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/fwd.hpp"
#include "glm/gtc/type_ptr.hpp"
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

#define MODEL_PATH "models/acidcannon/scene.gltf"

namespace {
constexpr auto kVkMajor = 1;
constexpr auto kVkMinor = 3;
constexpr auto kVkPatch = 0;

constexpr auto kVkVersionV = VK_MAKE_VERSION(kVkMajor, kVkMinor, kVkPatch);

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
      std::println("[vkit] [WARNING] Vulkan layer '{}' not found", layer);
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
    bindlessSetManager_->addTexture2D(*gpu_->device, static_cast<uint32_t>(idx),
                                      *texture);
  }

  size_t max_idx = 0;
  for (const auto& [idx, m] : gltfAsset_->materials) {
    max_idx = std::max(max_idx, idx);
  }
  materials_.clear();
  materials_.resize(max_idx + 1);

  for (const auto& [idx, m] : gltfAsset_->materials) {
    Material mat{};

    mat.baseColorFactor = m.baseColorFactor;
    mat.emissiveFactor = glm::vec4(glm::make_vec3(&m.emissiveFactor.x), 1.0F);
    mat.metallicFactor = m.metallicFactor;
    mat.roughnessFactor = m.roughnessFactor;
    mat.alphaMaskCutoff =
        (m.alphaMode == fastgltf::AlphaMode::Mask) ? m.alphaCutoff : 0.0F;
    mat.emissiveStrength = m.emissiveStrength;

    auto get_tex_idx = [](const std::optional<std::uint32_t>& tex) -> int32_t {
      return tex.has_value() ? static_cast<int32_t>(*tex) : -1;
    };

    mat.baseColorTextureIdx = get_tex_idx(m.baseColorTexture);
    mat.metallicRoughnessTextureIdx = get_tex_idx(m.metallicRoughnessTexture);
    mat.normalTextureIdx = get_tex_idx(m.normalTexture);
    mat.occlusionTextureIdx = get_tex_idx(m.occlusionTexture);
    mat.emissiveTextureIdx = get_tex_idx(m.emissiveTexture);

    materials_[idx] = mat;
  }

  updateMaterials();
}

void App::inspect() {
  ImGuiIO& io = ImGui::GetIO();
  ImGuizmo::BeginFrame();

  ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);
  ImGuizmo::SetDrawlist(ImGui::GetForegroundDrawList());

  glm::vec3 cam_pos = camera_.getPosition();
  glm::mat4 view = glm::lookAt(cam_pos, camera_.target, camera_.up);

  ImGuizmo::ViewManipulate(glm::value_ptr(view), camera_.distance,
                           ImVec2(io.DisplaySize.x - 128, 0), ImVec2(128, 128),
                           0x00000000);

  if (ImGuizmo::IsUsing() && io.MouseDown[0]) {
    glm::mat4 inv_view = glm::inverse(view);

    glm::vec3 new_cam_pos = glm::vec3(inv_view * glm::vec4(0, 0, 0, 1));

    glm::vec3 dir = new_cam_pos - camera_.target;
    camera_.distance = glm::length(dir);
    dir = glm::normalize(dir);

    camera_.pitch = glm::degrees(asin(glm::clamp(dir.y, -0.99F, 0.99F)));
    camera_.yaw = glm::degrees(atan2(dir.z, dir.x));
  }

  if (!io.WantCaptureMouse && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
    camera_.yaw =
        std::fmod(camera_.yaw + (io.MouseDelta.x * 0.5F) + 180.0F, 360.0F) -
        180.0F;
    camera_.pitch += io.MouseDelta.y * 0.5F;
    camera_.pitch = glm::clamp(camera_.pitch, -89.0F, 89.0F);
  }

  if (!io.WantCaptureMouse) {
    camera_.distance -= io.MouseWheel * 0.5F;
    camera_.distance = glm::max(camera_.distance, 0.1F);
  }

  ImGui::SetNextWindowSize({325.0F, 370.0F}, ImGuiCond_Once);
  if (ImGui::Begin("Inspect")) {
    if (ImGui::TreeNodeEx("Camera", ImGuiTreeNodeFlags_DefaultOpen)) {
      ImGui::DragFloat3("Target", glm::value_ptr(camera_.target), 0.1F);
      ImGui::DragFloat("Distance", &camera_.distance, 0.1F, 0.1F, 100.0F);
      ImGui::SliderFloat("Yaw", &camera_.yaw, -180.0F, 180.0F);
      ImGui::SliderFloat("Pitch", &camera_.pitch, -89.0F, 89.0F);
      ImGui::TreePop();
    }
    if (ImGui::TreeNodeEx("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
      ImGui::DragFloat3("Position", glm::value_ptr(transform_.position), 0.1F);

      glm::vec3 euler_rotation =
          glm::degrees(glm::eulerAngles(transform_.rotation));
      if (ImGui::DragFloat3("Rotation", glm::value_ptr(euler_rotation), 0.1F)) {
        transform_.rotation = glm::quat(glm::radians(euler_rotation));
      }

      ImGui::DragFloat3("Scale", glm::value_ptr(transform_.scale), 0.05F);

      ImGui::Separator();

      static bool show_gizmo = true;
      ImGui::Checkbox("Show Gizmo", &show_gizmo);

      if (show_gizmo) {
        static ImGuizmo::OPERATION current_op(ImGuizmo::TRANSLATE);

        if (ImGui::RadioButton("Translate", current_op == ImGuizmo::TRANSLATE))
          current_op = ImGuizmo::TRANSLATE;
        ImGui::SameLine();
        if (ImGui::RadioButton("Rotate", current_op == ImGuizmo::ROTATE))
          current_op = ImGuizmo::ROTATE;
        ImGui::SameLine();
        if (ImGui::RadioButton("Scale", current_op == ImGuizmo::SCALE))
          current_op = ImGuizmo::SCALE;

        glm::mat4 model = transform_.modelMatrix();
        glm::mat4 gizmo_view =
            glm::lookAt(camera_.getPosition(), camera_.target, camera_.up);
        glm::mat4 gizmo_proj = glm::perspective(
            glm::radians(90.0F), io.DisplaySize.x / io.DisplaySize.y, 0.1F,
            1000.0F);

        ImGuizmo::Manipulate(glm::value_ptr(gizmo_view),
                             glm::value_ptr(gizmo_proj), current_op,
                             ImGuizmo::LOCAL, glm::value_ptr(model));

        if (ImGuizmo::IsUsing()) {
          glm::vec3 new_pos;
          glm::vec3 new_scale;
          glm::vec3 new_euler;
          ImGuizmo::DecomposeMatrixToComponents(
              glm::value_ptr(model), glm::value_ptr(new_pos),
              glm::value_ptr(new_euler), glm::value_ptr(new_scale));

          transform_.position = new_pos;
          transform_.scale = new_scale;
          transform_.rotation = glm::quat(glm::radians(new_euler));
        }
      }

      ImGui::TreePop();
    }

    ImGui::Separator();
    if (ImGui::TreeNodeEx("Params", ImGuiTreeNodeFlags_DefaultOpen)) {
      ImGui::DragFloat3("LightDir", glm::value_ptr(uboParams_.lightDir), 0.01F);
      ImGui::DragFloat("Exposure", &uboParams_.exposure, 0.1F, 0.1F, 100.0F);
      ImGui::DragFloat("Gamma", &uboParams_.gamma, 0.1F, 0.1F, 100.0F);
      ImGui::TreePop();
    }
  }
  ImGui::End();
}

void App::update() {
  auto model = transform_.modelMatrix();
  auto cam_pos = camera_.getPosition();
  auto view = glm::lookAt(cam_pos, camera_.target, camera_.up);

  float aspect = static_cast<float>(frameBufferSize_.x) /
                 static_cast<float>(frameBufferSize_.y);
  auto projection =
      glm::perspective(glm::radians(90.0F), aspect, 0.1F, 1000.0F);

  ubo_ = UBO{
      .model = model,
      .view = view,
      .projection = projection,
      .cameraPosition = cam_pos,
  };

  const auto ubo_bytes =
      std::bit_cast<std::array<std::byte, sizeof(ubo_)>>(ubo_);
  uboBuffers_->writeAt(frameIndex_, ubo_bytes);

  const auto ubo_params_bytes =
      std::bit_cast<std::array<std::byte, sizeof(uboParams_)>>(uboParams_);
  uboParamsBuffers_->writeAt(frameIndex_, ubo_params_bytes);
}

void App::updateMaterials() {
  auto bytes = toByteSpan(materials_);
  for (uint32_t i = 0; i < kResourceBufferingV; ++i) {
    materialsBuffers_->writeAt(i, bytes);
  }
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

void App::draw(vk::CommandBuffer cb) const {
  if (!gltfAsset_.has_value()) {
    return;
  }

  bindDescriptorSets(cb);
  shader_->bind(cb, frameBufferSize_);

  cb.setDepthWriteEnable(vk::True);
  cb.setColorBlendEnableEXT(0, vk::False);
  cb.setCullMode(vk::CullModeFlagBits::eBack);

  fastgltf::iterateSceneNodes(gltfAsset_->asset, gltfAsset_->sceneIdx,
                              fastgltf::math::fmat4x4(),
                              [&](const fastgltf::Node& node,
                                  const fastgltf::math::fmat4x4& transform) {
                                drawNode(cb, node, transform, false);
                              });

  cb.setDepthWriteEnable(vk::False);

  auto color_blend_equation =
      vk::ColorBlendEquationEXT{}
          .setSrcColorBlendFactor(vk::BlendFactor::eSrcAlpha)
          .setDstColorBlendFactor(vk::BlendFactor::eOneMinusSrcAlpha)
          .setColorBlendOp(vk::BlendOp::eAdd)
          .setSrcAlphaBlendFactor(vk::BlendFactor::eOne)
          .setDstAlphaBlendFactor(vk::BlendFactor::eZero)
          .setAlphaBlendOp(vk::BlendOp::eAdd);

  cb.setColorBlendEnableEXT(0, vk::True);
  cb.setColorBlendEquationEXT(0, color_blend_equation);

  fastgltf::iterateSceneNodes(gltfAsset_->asset, gltfAsset_->sceneIdx,
                              fastgltf::math::fmat4x4(),
                              [&](const fastgltf::Node& node,
                                  const fastgltf::math::fmat4x4& transform) {
                                drawNode(cb, node, transform, true);
                              });
}

void App::drawNode(vk::CommandBuffer cb, const fastgltf::Node& node,
                   const fastgltf::math::fmat4x4& transform,
                   bool isTransparentPass) const {
  using PbrPushConstants = vkit::vulkan::pl::PBRPipelineLayout::PushConstants;

  if (!node.meshIndex.has_value()) return;

  auto it = gltfAsset_->meshes.find(node.meshIndex.value());
  assert(it != gltfAsset_->meshes.end());
  const auto& mesh = it->second;

  glm::mat4 node_transform;
  std::memcpy(glm::value_ptr(node_transform), transform.data(),
              sizeof(glm::mat4));

  auto push_constants = PbrPushConstants{
      .meshIdx = static_cast<uint32_t>(node.meshIndex.value()),
      .transform = node_transform,
  };

  for (const auto& primitive : mesh->primitives) {
    const auto& material = gltfAsset_->materials.at(primitive.materialIdx);
    bool is_blend = (material.alphaMode == fastgltf::AlphaMode::Blend);

    if (isTransparentPass && !is_blend) continue;
    if (!isTransparentPass && is_blend) continue;

    vk::CullModeFlags cull_mode = material.doubleSided
                                      ? vk::CullModeFlagBits::eNone
                                      : vk::CullModeFlagBits::eBack;
    cb.setCullMode(cull_mode);

    push_constants.materialIdx = primitive.materialIdx;
    push_constants.vertices = primitive.vertexBuffer.getAddress(*gpu_->device);

    cb.pushConstants(
        pbrPipelineLayout_->get(),
        vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
        0, sizeof(PbrPushConstants), &push_constants);

    cb.bindIndexBuffer(primitive.indexBuffer.buffer, 0, vk::IndexType::eUint32);
    cb.drawIndexed(primitive.indexCount, 1, 0, 0, 0);
  }
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
  const auto materials_info = materialsBuffers_->descriptorInfoAt(frameIndex_);
  write.setBufferInfo(materials_info)
      .setDescriptorType(vk::DescriptorType::eStorageBuffer)
      .setDescriptorCount(1)
      .setDstSet(materials_set)
      .setDstBinding(0);
  writes[2] = write;

  gpu_->device->updateDescriptorSets(writes, {});
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
      .setClearValue(vk::ClearColorValue{0.2F, 0.2F, 0.2F, 1.0F});

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

  cb.beginRendering(rendering_info);
  update();
  updateDescriptorSets();
  draw(cb);
  cb.endRendering();

  auto imgui_attachment = vk::RenderingAttachmentInfo{};
  imgui_attachment.setImageView(renderTarget_->swapchainImageView)
      .setImageLayout(vk::ImageLayout::eColorAttachmentOptimal)
      .setLoadOp(vk::AttachmentLoadOp::eLoad)
      .setStoreOp(vk::AttachmentStoreOp::eStore);

  auto imgui_rendering = vk::RenderingInfo{};
  imgui_rendering.setRenderArea(render_area)
      .setLayerCount(1)
      .setColorAttachmentCount(1)
      .setPColorAttachments(&imgui_attachment);

  cb.beginRendering(imgui_rendering);
  inspect();
  imgui_->endFrame();
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
  materialsBuffers_.emplace(gpu_->allocator, gpu_->queueFamilies.transfer,
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
  pool_ci.setPoolSizes(kPoolSizeV)
      .setMaxSets(16)
      .setFlags(vk::DescriptorPoolCreateFlagBits::eUpdateAfterBind |
                vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet);
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
    throw std::runtime_error{
        std::format("loader does not support vulkan {}.{}.{}", kVkMajor,
                    kVkMinor, kVkPatch),
    };
  }

  auto app_info =
      vk::ApplicationInfo{}.setPApplicationName("vkit").setApiVersion(
          kVkVersionV);

  auto debug_create_info =
      vk::DebugUtilsMessengerCreateInfoEXT{}
          .setMessageSeverity(
              vk::DebugUtilsMessageSeverityFlagBitsEXT::eError |
              vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
              vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo)
          .setMessageType(vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
                          vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
                          vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance)
          .setPfnUserCallback(&debugCallback);

  auto glfw_extensions = glfw::instanceExtensions();
  auto extensions =
      std::vector<const char*>(glfw_extensions.begin(), glfw_extensions.end());

  extensions.push_back(vk::EXTDebugUtilsExtensionName);

  static constexpr auto kLayersV = std::array{"VK_LAYER_KHRONOS_validation"};
  auto const layers = getLayers(kLayersV);

  auto instance_ci = vk::InstanceCreateInfo{}
                         .setPApplicationInfo(&app_info)
                         .setPEnabledLayerNames(layers)
                         .setPEnabledExtensionNames(extensions)
                         .setPNext(&debug_create_info);

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
