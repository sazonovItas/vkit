#include "app.hpp"

#include <backends/imgui_impl_glfw.h>
#include <imgui.h>

#include <algorithm>
#include <array>
#include <cassert>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <optional>
#include <print>
#include <ranges>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "GLFW/glfw3.h"
#include "dear_imgui.hpp"
#include "fastgltf/core.hpp"
#include "fastgltf/glm_element_traits.hpp"
#include "fastgltf/math.hpp"
#include "fastgltf/tools.hpp"
#include "fastgltf/types.hpp"
#include "glm/ext/matrix_clip_space.hpp"
#include "glm/ext/matrix_common.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/fwd.hpp"
#include "glm/glm.hpp"
#include "model.hpp"
#include "resource_buffering.hpp"
#include "shader_program.hpp"
#include "vku/buffers/device_buffer.hpp"
#include "vku/utils/utils.hpp"
#include "vulkan/gpu.hpp"
#include "vulkan/vulkan.hpp"
#include "window.hpp"

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE;

namespace {
constexpr auto kVkVersionV = VK_MAKE_VERSION(1, 3, 0);

VKAPI_ATTR VkBool32 VKAPI_CALL
debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
               VkDebugUtilsMessageTypeFlagsEXT messageType,
               const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
               void* pUserData) {
  if (pCallbackData && pCallbackData->pMessage) {
    std::println("[Validation]: {}", pCallbackData->pMessage);
  }

  return VK_FALSE;
}
[[nodiscard]] auto locate_assets_dir() -> fs::path {
  static constexpr std::string_view kDirNameV{"assets"};

  for (auto path = fs::current_path(); !path.empty() && path.has_parent_path();
       path = path.parent_path()) {
    auto ret = path / kDirNameV;
    if (fs::is_directory(ret)) {
      return ret;
    }
  }

  std::println("[lvk] warning could not locate '{}' directory", kDirNameV);
  return fs::current_path();
}

[[nodiscard]] auto get_layers(std::span<char const* const> desired)
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

[[nodiscard]] auto to_spir_v(fs::path const& path)
    -> std::vector<std::uint32_t> {
  auto file = std::ifstream{path, std::ios::binary | std::ios::ate};
  if (!file.is_open()) {
    throw std::runtime_error{
        std::format("failed to open file: '{}'", path.generic_string())};
  }

  auto const size = file.tellg();
  auto const usize = static_cast<std::uint64_t>(size);
  if (0 != usize % sizeof(std::uint32_t)) {
    throw std::runtime_error{std::format("invalid SPIR-V size: {}", usize)};
  }

  file.seekg({}, std::ios::beg);
  auto ret = std::vector<std::uint32_t>{};
  ret.resize(usize / sizeof(std::uint32_t));
  void* data = ret.data();
  file.read(static_cast<char*>(data), size);
  return ret;
}

template <typename T>
[[nodiscard]] constexpr auto to_byte_array(T const& t) {
  return std::bit_cast<std::array<std::byte, sizeof(T)>>(t);
}

template <typename T>
[[nodiscard]] constexpr auto to_byte_span(std::vector<T> const& v) {
  return std::as_bytes(std::span{v});
}

constexpr auto layout_binding(std::uint32_t binding,
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

BitmapData generate_bricks(int width, int height, float brickWidth,
                           float brickHeight, float mortarThickness) {
  BitmapData result;
  result.storage.resize(width * height * 4);

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
        result.storage[index + 0] = std::byte(mortar);
        result.storage[index + 1] = std::byte(mortar);
        result.storage[index + 2] = std::byte(mortar);
        result.storage[index + 3] = std::byte(255);
      } else {
        float noise = hash(static_cast<int>(std::floor(bx)),
                           static_cast<int>(std::floor(by)));
        float variation = 0.8F + (noise * 0.4F);

        auto r = static_cast<uint8_t>(150 * variation);
        auto g = static_cast<uint8_t>(50 * variation);
        auto b = static_cast<uint8_t>(40 * variation);

        result.storage[index + 0] = std::byte(r);
        result.storage[index + 1] = std::byte(g);
        result.storage[index + 2] = std::byte(b);
        result.storage[index + 3] = std::byte(255);
      }
    }
  }

  result.bitmap = {
      .extent =
          vk::Extent2D{
              static_cast<std::uint32_t>(width),
              static_cast<std::uint32_t>(height),
          },
      .bytes = std::span<const std::byte>(result.storage.data(),
                                          result.storage.size()),
  };

  return result;
}

BitmapData generateStoneTiles(int width, int height, int tileSize,
                              float mortarThickness) {
  BitmapData result;
  result.storage.resize(width * height * 4);

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
        result.storage[index + 0] = std::byte(mortar);
        result.storage[index + 1] = std::byte(mortar);
        result.storage[index + 2] = std::byte(mortar);
        result.storage[index + 3] = std::byte(255);
      } else {
        float base_noise = hash(tile_x, tile_y);
        float variation = 0.7F + (base_noise * 0.6F);

        // дополнительная мелкая зернистость
        float fine_noise = hash(x, y);
        float grain = 0.9F + (fine_noise * 0.2F);

        auto r = static_cast<uint8_t>(120 * variation * grain);
        auto g = static_cast<uint8_t>(110 * variation * grain);
        auto b = static_cast<uint8_t>(100 * variation * grain);

        result.storage[index + 0] = std::byte(r);
        result.storage[index + 1] = std::byte(g);
        result.storage[index + 2] = std::byte(b);
        result.storage[index + 3] = std::byte(255);
      }
    }
  }

  result.bitmap = {
      .extent =
          {
              static_cast<std::uint32_t>(width),
              static_cast<std::uint32_t>(height),
          },
      .bytes = std::span<const std::byte>(result.storage.data(),
                                          result.storage.size()),
  };

  return result;
}
};  // namespace

namespace lvk {
void App::run() {
  m_assets_dir_ = locate_assets_dir();

  create_window();
  create_instance();
  create_surface();
  create_device();
  create_swapchain();
  create_render_sync();
  create_imgui();
  create_descriptor_pool();
  create_pipeline_layout();
  create_shader();
  create_cmd_block_pool();
  create_transfer_command_pool();

  create_shader_resources();
  create_descriptor_sets();

  load_gltf();

  main_loop();
}

void App::main_loop() {
  while (glfwWindowShouldClose(m_window_.get()) == GLFW_FALSE) {
    glfwPollEvents();

    if (!acquire_render_target()) {
      continue;
    }

    auto const command_buffer = begin_frame();

    transition_for_render(command_buffer);
    render(command_buffer);

    transition_for_present(command_buffer);
    submit_and_present();
  }
}

void App::inspect() {
  ImGui::SetNextWindowSize({300.0F, 320.0F}, ImGuiCond_Once);
  if (ImGui::Begin("Inspect")) {
    if (ImGui::Checkbox("wireframe", &m_wireframe_)) {
      m_shader_->polygon_mode =
          m_wireframe_ ? vk::PolygonMode::eLine : vk::PolygonMode::eFill;
    }
    if (m_wireframe_) {
      auto const& line_width_range = m_gpu_->properties.limits.lineWidthRange;
      ImGui::SetNextItemWidth(100.0F);
      ImGui::DragFloat("line width", &m_line_width_, 0.25F, line_width_range[0],
                       line_width_range[1]);
      m_shader_->line_width = m_line_width_;
    }

    static auto const kCameraView = [](Camera& out) {
      ImGui::DragFloat3("position", &out.position.x);
      ImGui::DragFloat3("target", &out.target.x);
      ImGui::DragFloat3("up", &out.up.x);
    };

    ImGui::Separator();
    if (ImGui::TreeNode("View")) {
      kCameraView(m_camera_);
      ImGui::TreePop();
    }

    static auto const kInspectTransform = [](Transform& out) {
      ImGui::DragFloat3("position", &out.position.x);
      ImGui::DragFloat3("rotation", &out.rotation.x);
      ImGui::DragFloat3("scale", &out.scale.x, 0.1F);
    };

    ImGui::Separator();
    if (ImGui::TreeNode("Instance")) {
      kInspectTransform(m_transform_);
      ImGui::TreePop();
    }

    ImGui::Separator();
    if (ImGui::TreeNode("Texture")) {
      if (m_textures_ && !m_textures_->empty()) {
        const char* preview = (*m_textures_)[m_curr_tex_idx_].name.c_str();

        if (ImGui::BeginCombo("Current", preview)) {
          for (uint32_t i = 0; i < m_textures_->size(); ++i) {
            bool selected = (m_curr_tex_idx_ == i);

            if (ImGui::Selectable((*m_textures_)[i].name.c_str(), selected)) {
              m_curr_tex_idx_ = i;
            }

            if (selected) ImGui::SetItemDefaultFocus();
          }

          ImGui::EndCombo();
        }
      }

      ImGui::TreePop();
    }
  }
  ImGui::End();
}

void App::draw(vk::CommandBuffer const command_buffer) const {
  m_shader_->bind(command_buffer, glm::ivec2{}, m_framebuffer_size_);
  bind_descriptor_sets(command_buffer);

  fastgltf::iterateSceneNodes(
      m_asset_.value(), 0, fastgltf::math::fmat4x4(1.0F),
      [&](const fastgltf::Node& node,
          const fastgltf::math::fmat4x4 /*matrix*/) {
        if (node.meshIndex.has_value()) {
          draw_mesh(command_buffer, node.meshIndex.value());
        }
      });
}

void App::draw_mesh(vk::CommandBuffer const command_buffer,
                    const std::size_t mesh_idx) const {
  for (const auto& primitive : meshes_.at(mesh_idx).primitives) {
    auto constants = PushConstants{
        .transform = m_transform_.model_matrix(),
        .vertex_buffer = primitive.vertex_buffer.getAddress(*m_gpu_->device),
    };
    command_buffer.pushConstants(*m_pipeline_layout_,
                                 vk::ShaderStageFlagBits::eVertex, 0,
                                 sizeof(PushConstants), &constants);

    command_buffer.bindIndexBuffer(primitive.index_buffer.buffer, 0,
                                   vk::IndexType::eUint32);
    command_buffer.drawIndexed(
        primitive.draw.count, primitive.draw.instance_count,
        primitive.draw.first_index, primitive.draw.vertex_offset,
        primitive.draw.first_instance);
  }
}

auto App::acquire_render_target() -> bool {
  m_framebuffer_size_ = glfw::framebuffer_size(m_window_.get());
  if (m_framebuffer_size_.x <= 0 || m_framebuffer_size_.y <= 0) {
    return false;
  }

  auto& render_sync = m_render_sync_.at(m_frame_index_);

  static constexpr auto kFenceTimeoutV = static_cast<std::uint64_t>(
      std::chrono::nanoseconds{std::chrono::seconds{3}}.count());
  auto result = m_gpu_->device->waitForFences(*render_sync.drawn, vk::True,
                                              kFenceTimeoutV);
  if (result != vk::Result::eSuccess) {
    throw std::runtime_error{"failed to wait for render fence"};
  }

  m_render_target_ = m_swapchain_->acquire_next_image(*render_sync.draw);
  if (!m_render_target_) {
    m_swapchain_->recreate(m_framebuffer_size_);
    return false;
  }

  m_gpu_->device->resetFences(*render_sync.drawn);

  m_imgui_->new_frame();

  return true;
}

auto App::begin_frame() -> vk::CommandBuffer {
  auto const& render_sync = m_render_sync_.at(m_frame_index_);

  auto command_buffer_bi = vk::CommandBufferBeginInfo{};
  command_buffer_bi.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
  render_sync.command_buffer.begin(command_buffer_bi);
  return render_sync.command_buffer;
}

void App::transition_for_render(vk::CommandBuffer const command_buffer) const {
  auto dependency_info = vk::DependencyInfo{};

  auto color_barrier = m_swapchain_->base_color_barrier();
  color_barrier.setOldLayout(vk::ImageLayout::eUndefined)
      .setNewLayout(vk::ImageLayout::eAttachmentOptimal)
      .setSrcAccessMask(vk::AccessFlagBits2::eColorAttachmentRead |
                        vk::AccessFlagBits2::eColorAttachmentWrite)
      .setSrcStageMask(vk::PipelineStageFlagBits2::eColorAttachmentOutput)
      .setDstAccessMask(color_barrier.srcAccessMask)
      .setDstStageMask(color_barrier.srcStageMask);

  auto depth_barrier = m_swapchain_->base_depth_barrier();
  depth_barrier.setOldLayout(vk::ImageLayout::eUndefined)
      .setNewLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal)
      .setSrcAccessMask(vk::AccessFlagBits2::eNone)
      .setSrcStageMask(vk::PipelineStageFlagBits2::eTopOfPipe)
      .setDstAccessMask(vk::AccessFlagBits2::eDepthStencilAttachmentRead |
                        vk::AccessFlagBits2::eDepthStencilAttachmentWrite)
      .setDstStageMask(vk::PipelineStageFlagBits2::eEarlyFragmentTests |
                       vk::PipelineStageFlagBits2::eLateFragmentTests);

  auto swapchain_color_barrier = m_swapchain_->base_barrier();
  swapchain_color_barrier.setOldLayout(vk::ImageLayout::eUndefined)
      .setNewLayout(vk::ImageLayout::eAttachmentOptimal)
      .setSrcAccessMask(vk::AccessFlagBits2::eColorAttachmentRead |
                        vk::AccessFlagBits2::eColorAttachmentWrite)
      .setSrcStageMask(vk::PipelineStageFlagBits2::eColorAttachmentOutput)
      .setDstAccessMask(swapchain_color_barrier.srcAccessMask)
      .setDstStageMask(swapchain_color_barrier.srcStageMask);

  auto barriers =
      std::array{color_barrier, depth_barrier, swapchain_color_barrier};
  dependency_info.setImageMemoryBarriers(barriers);
  command_buffer.pipelineBarrier2(dependency_info);
}

void App::render(vk::CommandBuffer const command_buffer) {
  auto color_attachment = vk::RenderingAttachmentInfo{};
  color_attachment.setImageView(m_render_target_->color_image_view)
      .setImageLayout(vk::ImageLayout::eColorAttachmentOptimal)
      .setResolveMode(vk::ResolveModeFlagBits::eAverage)
      .setResolveImageView(m_render_target_->image_view)
      .setResolveImageLayout(vk::ImageLayout::eColorAttachmentOptimal)
      .setLoadOp(vk::AttachmentLoadOp::eClear)
      .setStoreOp(vk::AttachmentStoreOp::eDontCare)
      .setClearValue(vk::ClearColorValue{0.0F, 0.0F, 0.0F, 1.0F});

  auto depth_attachment = vk::RenderingAttachmentInfo{};
  depth_attachment.setImageView(m_render_target_->depth_image_view)
      .setImageLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal)
      .setLoadOp(vk::AttachmentLoadOp::eClear)
      .setStoreOp(vk::AttachmentStoreOp::eDontCare)
      .setClearValue(vk::ClearDepthStencilValue{1.0F, 0});

  auto rendering_info = vk::RenderingInfo{};
  auto const render_area = vk::Rect2D{vk::Offset2D{}, m_render_target_->extent};
  rendering_info.setRenderArea(render_area)
      .setLayerCount(1)
      .setColorAttachmentCount(1)
      .setPColorAttachments(&color_attachment)
      .setPDepthAttachment(&depth_attachment);

  command_buffer.beginRendering(rendering_info);
  update_view();
  update_instances();
  draw(command_buffer);
  command_buffer.endRendering();

  auto imgui_attachment = vk::RenderingAttachmentInfo{};
  imgui_attachment.setImageView(m_render_target_->image_view)
      .setResolveImageLayout(vk::ImageLayout::eColorAttachmentOptimal)
      .setLoadOp(vk::AttachmentLoadOp::eLoad)
      .setStoreOp(vk::AttachmentStoreOp::eStore);

  auto imgui_rendering = vk::RenderingInfo{};
  imgui_rendering.setRenderArea(render_area)
      .setLayerCount(1)
      .setColorAttachmentCount(1)
      .setPColorAttachments(&imgui_attachment);

  command_buffer.beginRendering(imgui_rendering);
  inspect();
  m_imgui_->end_frame();
  m_imgui_->render(command_buffer);
  command_buffer.endRendering();
}

void App::transition_for_present(vk::CommandBuffer const command_buffer) const {
  auto barrier = m_swapchain_->base_barrier();
  barrier.setOldLayout(vk::ImageLayout::eAttachmentOptimal)
      .setNewLayout(vk::ImageLayout::ePresentSrcKHR)
      .setSrcAccessMask(vk::AccessFlagBits2::eColorAttachmentRead |
                        vk::AccessFlagBits2::eColorAttachmentWrite)
      .setSrcStageMask(vk::PipelineStageFlagBits2::eColorAttachmentOutput)
      .setDstAccessMask(barrier.srcAccessMask)
      .setDstStageMask(barrier.srcStageMask);

  auto dependency_info = vk::DependencyInfo{};
  dependency_info.setImageMemoryBarriers(barrier);

  command_buffer.pipelineBarrier2(dependency_info);
}

void App::submit_and_present() {
  auto const& render_sync = m_render_sync_.at(m_frame_index_);
  render_sync.command_buffer.end();

  auto submit_info = vk::SubmitInfo2{};
  auto const command_buffer_info =
      vk::CommandBufferSubmitInfo{render_sync.command_buffer};

  auto wait_semaphore_info = vk::SemaphoreSubmitInfo{};
  wait_semaphore_info.setSemaphore(*render_sync.draw)
      .setStageMask(vk::PipelineStageFlagBits2::eColorAttachmentOutput);

  auto signal_semaphore_info = vk::SemaphoreSubmitInfo{};
  signal_semaphore_info.setSemaphore(m_swapchain_->get_present_semaphore())
      .setStageMask(vk::PipelineStageFlagBits2::eColorAttachmentOutput);

  submit_info.setCommandBufferInfos(command_buffer_info)
      .setWaitSemaphoreInfos(wait_semaphore_info)
      .setSignalSemaphoreInfos(signal_semaphore_info);

  m_gpu_->queues.graphicsPresent.submit2(submit_info, *render_sync.drawn);

  m_frame_index_ = (m_frame_index_ + 1) % m_render_sync_.size();
  m_render_target_.reset();

  auto const fb_size_changed = m_framebuffer_size_ != m_swapchain_->get_size();
  auto const out_of_date =
      !m_swapchain_->present(m_gpu_->queues.graphicsPresent);
  if (fb_size_changed || out_of_date) {
    m_swapchain_->recreate(m_framebuffer_size_);
  }
}

void App::update_instances() {}

void App::update_view() {
  auto const half_size = 0.5F * glm::vec2{m_framebuffer_size_};
  auto mat_projection =
      glm::perspective(glm::radians(90.0F),
                       static_cast<float>(m_framebuffer_size_.x) /
                           static_cast<float>(m_framebuffer_size_.y),
                       0.1F, 1000.0F);

  auto const mat_view =
      glm::lookAt(m_camera_.position, m_camera_.target, m_camera_.up);

  auto const mat_vp = mat_projection * mat_view;

  auto const bytes =
      std::bit_cast<std::array<std::byte, sizeof(mat_vp)>>(mat_vp);

  m_view_ubo_->write_at(m_frame_index_, bytes);
}

void App::bind_descriptor_sets(vk::CommandBuffer const command_buffer) const {
  auto writes = std::array<vk::WriteDescriptorSet, 2>{};

  auto const& descriptor_sets = m_descriptor_sets_.at(m_frame_index_);
  auto write = vk::WriteDescriptorSet{};

  auto const set0 = descriptor_sets[0];
  auto const view_ubo_info = m_view_ubo_->descriptorInfoAt(m_frame_index_);
  write.setBufferInfo(view_ubo_info)
      .setDescriptorType(vk::DescriptorType::eUniformBuffer)
      .setDescriptorCount(1)
      .setDstSet(set0)
      .setDstBinding(0);
  writes[0] = write;

  auto const image_info = (*m_textures_)[m_curr_tex_idx_].descriptorInfo();
  write.setImageInfo(image_info)
      .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
      .setDescriptorCount(1)
      .setDstSet(set0)
      .setDstBinding(1);
  writes[1] = write;

  m_gpu_->device->updateDescriptorSets(writes, {});

  command_buffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
                                    *m_pipeline_layout_, 0, descriptor_sets,
                                    {});
}

void App::create_shader_resources() {
  m_view_ubo_.emplace(m_gpu_->allocator, m_gpu_->queueFamilies.graphicsPresent,
                      vk::BufferUsageFlagBits::eUniformBuffer);

  using Pixel = std::array<std::byte, 4>;
  using std::vector;
  using vku::Bitmap;

  m_textures_.emplace(std::vector<Texture>{});

  static constexpr auto kRgbaPixelsV = std::array{
      Pixel{std::byte{0xFF}, {}, {}, std::byte{0xFF}},
      Pixel{std::byte{}, std::byte{0xFF}, {}, std::byte{0xFF}},
      Pixel{std::byte{}, {}, std::byte{0xFF}, std::byte{0xFF}},
      Pixel{std::byte{0xFF}, std::byte{0xFF}, {}, std::byte{0xFF}},
  };
  static constexpr auto kRgbaBytesV =
      std::bit_cast<std::array<std::byte, sizeof(kRgbaPixelsV)>>(kRgbaPixelsV);

  // 4xSquare board texture
  static constexpr auto kRgbyBitmapV = Bitmap{
      .extent = vk::Extent2D{2, 2},
      .bytes = kRgbaBytesV,
  };
  auto grb_board_ci = Texture::CreateInfo{
      .name = "4xSquare board",
      .format = vk::Format::eR8G8B8A8Srgb,
      .bitmap = kRgbyBitmapV,
      .allocator = m_gpu_->allocator,
      .device = *m_gpu_->device,
      .commandPool = *m_cmd_block_pool_,
      .queue = m_gpu_->queues.graphicsPresent,
  };
  grb_board_ci.sampler.setMagFilter(vk::Filter::eNearest);

  m_textures_->emplace_back(grb_board_ci);

  // Bricks texture
  auto bricks = generate_bricks(1024, 1024, 8.0F, 8.0F, 0.125F);
  auto bricks_ci = Texture::CreateInfo{
      .name = "Bricks",
      .format = vk::Format::eR8G8B8A8Srgb,
      .bitmap = bricks.bitmap,
      .allocator = m_gpu_->allocator,
      .device = *m_gpu_->device,
      .commandPool = *m_cmd_block_pool_,
      .queue = m_gpu_->queues.graphicsPresent,
  };
  bricks_ci.sampler.setMagFilter(vk::Filter::eNearest);

  m_textures_->emplace_back(bricks_ci);

  // Stone texture
  auto stone = generateStoneTiles(128, 128, 8, 0.5F);
  auto stone_ci = Texture::CreateInfo{
      .name = "Stone",
      .format = vk::Format::eR8G8B8A8Srgb,
      .bitmap = stone.bitmap,
      .allocator = m_gpu_->allocator,
      .device = *m_gpu_->device,
      .commandPool = *m_cmd_block_pool_,
      .queue = m_gpu_->queues.graphicsPresent,
  };
  stone_ci.sampler.setMagFilter(vk::Filter::eNearest);

  m_textures_->emplace_back(stone_ci);
}

void App::create_descriptor_sets() {
  for (auto& descriptor_set : m_descriptor_sets_) {
    descriptor_set = allocate_sets();
  }
}

auto App::allocate_sets() const -> std::vector<vk::DescriptorSet> {
  auto allocate_info = vk::DescriptorSetAllocateInfo{};
  allocate_info.setDescriptorPool(*m_descriptor_pool_)
      .setSetLayouts(m_set_layout_views_);
  return m_gpu_->device->allocateDescriptorSets(allocate_info);
}

void App::create_pipeline_layout() {
  static constexpr auto kSet0BindingsV = std::array{
      layout_binding(0, vk::DescriptorType::eUniformBuffer),
      layout_binding(1, vk::DescriptorType::eCombinedImageSampler),
  };

  auto set_layout_cis = std::array<vk::DescriptorSetLayoutCreateInfo, 1>{};
  set_layout_cis[0].setBindings(kSet0BindingsV);

  for (auto const& set_layout_ci : set_layout_cis) {
    m_set_layouts_.push_back(
        m_gpu_->device->createDescriptorSetLayoutUnique(set_layout_ci));
    m_set_layout_views_.push_back(*m_set_layouts_.back());
  }

  auto push_constants_ranges = std::array<vk::PushConstantRange, 1>{};
  push_constants_ranges[0]
      .setOffset(0)
      .setSize(sizeof(PushConstants))
      .setStageFlags(vk::ShaderStageFlagBits::eVertex);

  for (const auto& push_constant_range : push_constants_ranges) {
    m_push_constant_ranges_.push_back(push_constant_range);
  }

  auto pipeline_layout_ci = vk::PipelineLayoutCreateInfo{};
  pipeline_layout_ci.setSetLayouts(m_set_layout_views_)
      .setPushConstantRanges(push_constants_ranges);
  m_pipeline_layout_ =
      m_gpu_->device->createPipelineLayoutUnique(pipeline_layout_ci);
}

void App::create_descriptor_pool() {
  static constexpr auto kPoolSizeV = std::array{
      vk::DescriptorPoolSize{vk::DescriptorType::eUniformBuffer,
                             kResourceBufferingV},
      vk::DescriptorPoolSize{vk::DescriptorType::eCombinedImageSampler,
                             kResourceBufferingV},
  };
  auto pool_ci = vk::DescriptorPoolCreateInfo{};
  pool_ci.setPoolSizes(kPoolSizeV).setMaxSets(16);
  m_descriptor_pool_ = m_gpu_->device->createDescriptorPoolUnique(pool_ci);
}

void App::create_transfer_command_pool() {
  transferCommnadPool_ =
      m_gpu_->createCommandPool(m_gpu_->queueFamilies.transfer,
                                vk::CommandPoolCreateFlagBits::eTransient);
}

void App::create_cmd_block_pool() {
  m_cmd_block_pool_ =
      m_gpu_->createCommandPool(m_gpu_->queueFamilies.graphicsPresent,
                                vk::CommandPoolCreateFlagBits::eTransient);
}

void App::create_shader() {
  auto const vertex_spirv = to_spir_v(asset_path("shaders/mesh.vert"));
  auto const fragment_spirv = to_spir_v(asset_path("shaders/mesh.frag"));
  auto const shader_ci = ShaderProgram::CreateInfo{
      .device = *m_gpu_->device,
      .vertex_spirv = vertex_spirv,
      .fragment_spirv = fragment_spirv,
      .set_layouts = m_set_layout_views_,
      .push_constant_ranges = m_push_constant_ranges_,
  };
  m_shader_.emplace(shader_ci);
}

void App::create_render_sync() {
  m_render_cmd_pool_ =
      m_gpu_->createCommandPool(m_gpu_->queueFamilies.graphicsPresent);

  auto command_buffer_ai = vk::CommandBufferAllocateInfo{};
  command_buffer_ai.setCommandPool(*m_render_cmd_pool_)
      .setCommandBufferCount(static_cast<std::uint32_t>(kResourceBufferingV))
      .setLevel(vk::CommandBufferLevel::ePrimary);
  auto const command_buffers =
      m_gpu_->device->allocateCommandBuffers(command_buffer_ai);
  assert(command_buffers.size() == m_render_sync_.size());

  static constexpr auto kFenceCreateInfoV =
      vk::FenceCreateInfo{vk::FenceCreateFlagBits::eSignaled};
  for (auto [sync, command_buffer] :
       std::ranges::views::zip(m_render_sync_, command_buffers)) {
    sync.command_buffer = command_buffer;
    sync.draw = m_gpu_->device->createSemaphoreUnique({});
    sync.drawn = m_gpu_->device->createFenceUnique(kFenceCreateInfoV);
  }
}

void App::create_imgui() {
  auto const imgui_ci = DearImGui::CreateInfo{
      .window = m_window_.get(),
      .api_vesrion = kVkVersionV,
      .instance = *m_instance_,
      .physical_device = m_gpu_->physicalDevice,
      .queue_family = m_gpu_->queueFamilies.graphicsPresent,
      .device = *m_gpu_->device,
      .queue = m_gpu_->queues.graphicsPresent,
      .color_format = m_swapchain_->get_format(),
      .samples = vk::SampleCountFlagBits::e1,
  };
  m_imgui_.emplace(imgui_ci);
}

void App::create_swapchain() {
  auto const size = glfw::framebuffer_size(m_window_.get());
  m_swapchain_.emplace(*m_gpu_->device, m_gpu_->physicalDevice,
                       m_gpu_->queueFamilies, m_gpu_->allocator, *m_surface_,
                       size);
}

void App::create_device() {
  m_gpu_.emplace(m_instance_.get(), m_surface_.get());
  m_waiter_ = *m_gpu_->device;
}

void App::create_surface() {
  m_surface_ = glfw::create_surface(m_window_.get(), *m_instance_);
}

void App::create_instance() {
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
  auto const layers = get_layers(kLayersV);

  auto glfw_extensions = glfw::instance_extensions();
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
  debug_create_info.setPfnUserCallback(debug_callback);

  auto instance_ci = vk::InstanceCreateInfo{};
  instance_ci.setPApplicationInfo(&app_info)
      .setPEnabledExtensionNames(extensions)
      .setPNext(debug_create_info);

  m_instance_ = vk::createInstanceUnique(instance_ci);
  VULKAN_HPP_DEFAULT_DISPATCHER.init(*m_instance_);

  m_debug_messenger_ = m_instance_->createDebugUtilsMessengerEXTUnique(
      debug_create_info, nullptr, VULKAN_HPP_DEFAULT_DISPATCHER);
}

void App::create_window() {
  m_window_ = glfw::create_window({1280, 720}, "learn vulkan");
}

auto App::asset_path(std::string_view uri) const -> fs::path {
  return m_assets_dir_ / uri;
}

void App::load_gltf() {
  if (!load_asset(asset_path("models/sword/scene.gltf"))) {
    std::println(stderr, "failed to load gltf model");
    return;
  }

  for (const auto& image : m_asset_->images) {
    load_image(image);
  }

  for (const auto& mesh : m_asset_->meshes) {
    load_mesh(mesh);
  }
}

auto App::load_asset(fs::path const& path) -> bool {
  if (!fs::exists(path)) {
    std::println(stderr, "failed to find file: `{}`", path.generic_string());
    return false;
  }

  std::println("Loading gltf from file: `{}`", path.generic_string());

  {
    static constexpr auto kSupportedExtensions =
        fastgltf::Extensions::KHR_mesh_quantization |
        fastgltf::Extensions::KHR_texture_transform |
        fastgltf::Extensions::KHR_materials_variants;

    fastgltf::Parser parser{kSupportedExtensions};

    static constexpr auto kGltfOptions =
        fastgltf::Options::DontRequireValidAssetMember |
        fastgltf::Options::AllowDouble |
        fastgltf::Options::LoadExternalBuffers |
        fastgltf::Options::LoadExternalImages |
        fastgltf::Options::GenerateMeshIndices;

    auto gltf_file = fastgltf::MappedGltfFile::FromPath(path);
    if (!static_cast<bool>(gltf_file)) {
      std::println(stderr, "failed to open glTF file: {}",
                   fastgltf::getErrorMessage(gltf_file.error()));
      return false;
    }

    auto expected_asset =
        parser.loadGltf(gltf_file.get(), path.parent_path(), kGltfOptions);
    if (expected_asset.error() != fastgltf::Error::None) {
      std::println(stderr, "failed to load glTF: {}",
                   fastgltf::getErrorMessage(expected_asset.error()));
      return false;
    }

    m_asset_.emplace(std::move(expected_asset.get()));
  }

  return true;
}

auto App::load_image(const fastgltf::Image& image) -> bool {
  assert(m_asset_.has_value());
  auto const& asset = *m_asset_;

  std::vector<std::byte> bytes;
  vku::Bitmap bitmap{};

  std::visit(fastgltf::visitor{
                 [&](fastgltf::sources::URI const& path) {},
                 [&](fastgltf::sources::Array const& array) {},
                 [&](fastgltf::sources::BufferView const& view) {},
                 [](auto& a) {},
             },
             image.data);

  return true;
}

auto App::load_mesh(const fastgltf::Mesh& mesh) -> bool {
  assert(m_asset_.has_value());
  auto const& asset = *m_asset_;

  auto out_mesh = Mesh{};
  out_mesh.primitives.resize(mesh.primitives.size());

  for (int i = 0; i < mesh.primitives.size(); i++) {
    const auto& primitive = mesh.primitives[i];

    assert(primitive.indicesAccessor.has_value());

    auto& out_primitive = out_mesh.primitives[i];

    auto vertices = std::vector<Vertex>{};
    auto indices = std::vector<std::uint32_t>{};

    // POSITIONS
    {
      const auto* pos_it = primitive.findAttribute("POSITION");
      assert(pos_it != primitive.attributes.end());

      const auto& pos_accessor = asset.accessors[pos_it->accessorIndex];
      if (!pos_accessor.bufferViewIndex.has_value()) continue;

      vertices.resize(pos_accessor.count);

      fastgltf::iterateAccessorWithIndex<glm::vec3>(
          asset, pos_accessor, [&](glm::vec3 pos, std::size_t idx) {
            vertices[idx].position = pos;
            vertices[idx].uv_x = 0.0F;
            vertices[idx].normal = glm::vec3(0.0F);
            vertices[idx].uv_y = 0.0F;
            vertices[idx].tangent = glm::vec4(0.0F);
          });
    }

    // NORMALS
    {
      const auto* normal_it = primitive.findAttribute("NORMAL");
      assert(normal_it != primitive.attributes.end());

      const auto& normal_accessor = asset.accessors[normal_it->accessorIndex];
      assert(normal_accessor.bufferViewIndex.has_value());

      fastgltf::iterateAccessorWithIndex<glm::vec3>(
          asset, normal_accessor, [&](glm::vec3 normal, std::size_t idx) {
            vertices[idx].normal = normal;
          });
    }

    // TANGENTS
    {
      const auto* tangent_it = primitive.findAttribute("TANGENT");
      if (tangent_it != primitive.attributes.end()) {
        const auto& tangent_accessor =
            asset.accessors[tangent_it->accessorIndex];
        assert(tangent_accessor.bufferViewIndex.has_value());

        fastgltf::iterateAccessorWithIndex<glm::vec4>(
            asset, tangent_accessor, [&](glm::vec4 tangent, std::size_t idx) {
              vertices[idx].tangent = tangent;
            });
      }
    }

    // TEXTURE COORDINATES
    {
      auto texcoord_attr = std::string("TEXCOORD_0");
      const auto* texcoord_it = primitive.findAttribute(texcoord_attr);
      assert(texcoord_it != primitive.attributes.end());

      const auto& texcoord_accessor =
          asset.accessors[texcoord_it->accessorIndex];
      assert(texcoord_accessor.bufferViewIndex.has_value());

      fastgltf::iterateAccessorWithIndex<glm::vec2>(
          asset, texcoord_accessor, [&](glm::vec2 uv, std::size_t idx) {
            vertices[idx].uv_x = uv.x;
            vertices[idx].uv_y = uv.y;
          });
    }

    // INDICES
    {
      assert(primitive.indicesAccessor.has_value());
      const auto& index_accessor =
          asset.accessors[primitive.indicesAccessor.value()];

      assert(index_accessor.bufferViewIndex.has_value());
      assert(index_accessor.componentType ==
             fastgltf::ComponentType::UnsignedInt);

      indices.resize(index_accessor.count);
      fastgltf::copyFromAccessor<std::uint32_t>(asset, index_accessor,
                                                indices.data());
    }

    // vertex buffer
    auto vertex_buffer = vku::DeviceBuffer{
        m_gpu_->allocator,
        vku::DeviceCopyInfo{
            .device = *m_gpu_->device,
            .commandPool = *transferCommnadPool_,
            .queue = m_gpu_->queues.transfer,
        },
        std::from_range_t{},
        vertices,
        vk::BufferUsageFlagBits::eShaderDeviceAddress,
    };
    out_primitive.vertex_buffer = std::move(vertex_buffer);

    auto index_buffer = vku::DeviceBuffer{
        m_gpu_->allocator,
        vku::DeviceCopyInfo{
            .device = *m_gpu_->device,
            .commandPool = *transferCommnadPool_,
            .queue = m_gpu_->queues.transfer,
        },
        std::from_range_t{},
        indices,
        vk::BufferUsageFlagBits::eIndexBuffer,
    };
    out_primitive.index_buffer = std::move(index_buffer);

    // draw command
    auto& draw = out_primitive.draw;
    draw.count = indices.size();
    draw.instance_count = 1;
    draw.first_index = 0;
    draw.vertex_offset = 0;
    draw.first_instance = 0;
  }

  meshes_.emplace_back(std::move(out_mesh));

  return true;
};

}  // namespace lvk
