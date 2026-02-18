#include "app.hpp"

#include <backends/imgui_impl_glfw.h>
#include <imgui.h>

#include <algorithm>
#include <array>
#include <cassert>
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <optional>
#include <print>
#include <ranges>
#include <stdexcept>
#include <string>
#include <vector>

#include "GLFW/glfw3.h"
#include "command_block.hpp"
#include "dear_imgui.hpp"
#include "fastgltf/core.hpp"
#include "fastgltf/glm_element_traits.hpp"
#include "fastgltf/tools.hpp"
#include "fastgltf/types.hpp"
#include "glm/ext/matrix_clip_space.hpp"
#include "glm/ext/matrix_common.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/fwd.hpp"
#include "glm/glm.hpp"
#include "gpu.hpp"
#include "model.hpp"
#include "resource_buffering.hpp"
#include "shader_program.hpp"
#include "vma.hpp"
#include "vulkan/vulkan.hpp"
#include "vulkan/vulkan_core.h"
#include "vulkan/vulkan_hpp_macros.hpp"
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
};  // namespace

namespace lvk {
void App::run() {
  m_assets_dir_ = locate_assets_dir();

  create_window();
  create_instance();
  create_surface();
  select_gpu();
  create_device();
  create_allocator();
  create_swapchain();
  create_depth_image();
  create_render_sync();
  create_imgui();
  create_descriptor_pool();
  create_pipeline_layout();
  create_shader();
  create_cmd_block_pool();

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
  ImGui::SetNextWindowSize({200.0F, 100.0F}, ImGuiCond_Once);
  if (ImGui::Begin("Inspect")) {
    if (ImGui::Checkbox("wireframe", &m_wireframe_)) {
      m_shader_->polygon_mode =
          m_wireframe_ ? vk::PolygonMode::eLine : vk::PolygonMode::eFill;
    }
    if (m_wireframe_) {
      auto const& line_width_range = m_gpu_.properties.limits.lineWidthRange;
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
    if (ImGui::TreeNode("Instances")) {
      for (std::size_t i = 0; i < m_instances_.size(); ++i) {
        auto const label = std::to_string(i);
        if (ImGui::TreeNode(label.c_str())) {
          kInspectTransform(m_instances_.at(i));
          ImGui::TreePop();
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
  for (const auto& mesh : meshes_) {
    draw_mesh(command_buffer, mesh);
  }
};

void App::draw_mesh(vk::CommandBuffer const command_buffer,
                    Mesh const& mesh) const {
  for (const auto& primitive : mesh.primitives) {
    auto constants = PushConstants{
        .vertex_buffer = primitive.vertex_buffer.get().address,
    };
    command_buffer.pushConstants(*m_pipeline_layout_,
                                 vk::ShaderStageFlagBits::eVertex, 0,
                                 sizeof(PushConstants), &constants);
    command_buffer.bindIndexBuffer(primitive.index_buffer.get().buffer, 0,
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
  auto result =
      m_device_->waitForFences(*render_sync.drawn, vk::True, kFenceTimeoutV);
  if (result != vk::Result::eSuccess) {
    throw std::runtime_error{"failed to wait for render fence"};
  }

  m_render_target_ = m_swapchain_->acquire_next_image(*render_sync.draw);
  if (!m_render_target_) {
    m_swapchain_->recreate(m_framebuffer_size_);
    return false;
  }

  m_device_->resetFences(*render_sync.drawn);

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

  auto depth_barrier = m_swapchain_->base_depth_barrier();
  depth_barrier.setOldLayout(vk::ImageLayout::eUndefined)
      .setNewLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal)
      .setSrcAccessMask(vk::AccessFlagBits2::eNone)
      .setSrcStageMask(vk::PipelineStageFlagBits2::eTopOfPipe)
      .setDstAccessMask(vk::AccessFlagBits2::eDepthStencilAttachmentRead |
                        vk::AccessFlagBits2::eDepthStencilAttachmentWrite)
      .setDstStageMask(vk::PipelineStageFlagBits2::eEarlyFragmentTests |
                       vk::PipelineStageFlagBits2::eLateFragmentTests);

  auto color_barrier = m_swapchain_->base_barrier();
  color_barrier.setOldLayout(vk::ImageLayout::eUndefined)
      .setNewLayout(vk::ImageLayout::eAttachmentOptimal)
      .setSrcAccessMask(vk::AccessFlagBits2::eColorAttachmentRead |
                        vk::AccessFlagBits2::eColorAttachmentWrite)
      .setSrcStageMask(vk::PipelineStageFlagBits2::eColorAttachmentOutput)
      .setDstAccessMask(color_barrier.srcAccessMask)
      .setDstStageMask(color_barrier.srcStageMask);

  auto barriers = std::array{depth_barrier, color_barrier};
  dependency_info.setImageMemoryBarriers(barriers);
  command_buffer.pipelineBarrier2(dependency_info);
}

void App::render(vk::CommandBuffer const command_buffer) {
  auto color_attachment = vk::RenderingAttachmentInfo{};
  color_attachment.setImageView(m_render_target_->image_view)
      .setImageLayout(vk::ImageLayout::eAttachmentOptimal)
      .setLoadOp(vk::AttachmentLoadOp::eClear)
      .setStoreOp(vk::AttachmentStoreOp::eStore)
      .setClearValue(vk::ClearColorValue{0.0F, 0.0F, 0.0F, 1.0F});

  auto depth_attachment = vk::RenderingAttachmentInfo{};
  depth_attachment.setImageView(m_render_target_->depth_image_view)
      .setImageLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal)
      .setLoadOp(vk::AttachmentLoadOp::eClear)
      .setStoreOp(vk::AttachmentStoreOp::eStore)
      .setClearValue(vk::ClearDepthStencilValue{1.0F, 0});

  auto rendering_info = vk::RenderingInfo{};
  auto const render_area = vk::Rect2D{vk::Offset2D{}, m_render_target_->extent};
  rendering_info.setRenderArea(render_area)
      .setLayerCount(1)
      .setColorAttachmentCount(1)
      .setPColorAttachments(&color_attachment)
      .setPDepthAttachment(&depth_attachment);

  update_view();
  update_instances();

  command_buffer.beginRendering(rendering_info);
  draw(command_buffer);
  command_buffer.endRendering();

  inspect();
  m_imgui_->end_frame();

  color_attachment.setLoadOp(vk::AttachmentLoadOp::eLoad);
  rendering_info.setColorAttachmentCount(1)
      .setPColorAttachments(&color_attachment)
      .setPDepthAttachment(nullptr);
  command_buffer.beginRendering(rendering_info);
  m_imgui_->render(command_buffer);
  command_buffer.endRendering();
}

void App::transition_for_present(vk::CommandBuffer const command_buffer) const {
  auto dependency_info = vk::DependencyInfo{};
  auto barrier = m_swapchain_->base_barrier();

  barrier.setOldLayout(vk::ImageLayout::eAttachmentOptimal)
      .setNewLayout(vk::ImageLayout::ePresentSrcKHR)
      .setSrcAccessMask(vk::AccessFlagBits2::eColorAttachmentRead |
                        vk::AccessFlagBits2::eColorAttachmentWrite)
      .setSrcStageMask(vk::PipelineStageFlagBits2::eColorAttachmentOutput)
      .setDstAccessMask(barrier.srcAccessMask)
      .setDstStageMask(barrier.srcStageMask);
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
      .setSemaphore(m_swapchain_->get_present_semaphore())
      .setStageMask(vk::PipelineStageFlagBits2::eColorAttachmentOutput);

  submit_info.setCommandBufferInfos(command_buffer_info)
      .setWaitSemaphoreInfos(wait_semaphore_info)
      .setSignalSemaphoreInfos(signal_semaphore_info);

  m_queue_.submit2(submit_info, *render_sync.drawn);

  m_frame_index_ = (m_frame_index_ + 1) % m_render_sync_.size();
  m_render_target_.reset();

  auto const fb_size_changed = m_framebuffer_size_ != m_swapchain_->get_size();
  auto const out_of_date = !m_swapchain_->present(m_queue_);
  if (fb_size_changed || out_of_date) {
    m_swapchain_->recreate(m_framebuffer_size_);
  }
}

void App::update_instances() {
  m_instance_data_.clear();
  m_instance_data_.reserve(m_instances_.size());
  for (auto const& transform : m_instances_) {
    m_instance_data_.push_back(transform.model_matrix());
  }

  auto const span = std::span{m_instance_data_};
  void* data = span.data();
  auto const bytes =
      std::span{static_cast<std::byte const*>(data), span.size_bytes()};
  m_instance_ssbo_->write_at(m_frame_index_, bytes);
}

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
  auto writes = std::array<vk::WriteDescriptorSet, 3>{};

  auto const& descriptor_sets = m_descriptor_sets_.at(m_frame_index_);
  auto write = vk::WriteDescriptorSet{};

  auto const set0 = descriptor_sets[0];
  auto const view_ubo_info = m_view_ubo_->descriptor_info_at(m_frame_index_);
  write.setBufferInfo(view_ubo_info)
      .setDescriptorType(vk::DescriptorType::eUniformBuffer)
      .setDescriptorCount(1)
      .setDstSet(set0)
      .setDstBinding(0);
  writes[0] = write;

  auto const set1 = descriptor_sets[1];
  auto const image_info = m_texture_->descriptor_info();
  write.setImageInfo(image_info)
      .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
      .setDescriptorCount(1)
      .setDstSet(set1)
      .setDstBinding(0);
  writes[1] = write;

  auto const set2 = descriptor_sets[2];
  auto const instance_ssbo_info =
      m_instance_ssbo_->descriptor_info_at(m_frame_index_);
  write.setBufferInfo(instance_ssbo_info)
      .setDescriptorType(vk::DescriptorType::eStorageBuffer)
      .setDescriptorCount(1)
      .setDstSet(set2)
      .setDstBinding(0);
  writes[2] = write;

  m_device_->updateDescriptorSets(writes, {});

  command_buffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
                                    *m_pipeline_layout_, 0, descriptor_sets,
                                    {});
}

void App::create_shader_resources() {
  m_view_ubo_.emplace(m_allocator_.get(), m_gpu_.queue_family,
                      vk::BufferUsageFlagBits::eUniformBuffer);

  m_instance_ssbo_.emplace(m_allocator_.get(), m_gpu_.queue_family,
                           vk::BufferUsageFlagBits::eStorageBuffer);

  using Pixel = std::array<std::byte, 4>;
  using vma::Bitmap;

  static constexpr auto kRgbaPixelsV = std::array{
      Pixel{std::byte{0xFF}, {}, {}, std::byte{0xFF}},
      Pixel{std::byte{}, std::byte{0xFF}, {}, std::byte{0xFF}},
      Pixel{std::byte{}, {}, std::byte{0xFF}, std::byte{0xFF}},
      Pixel{std::byte{0xFF}, std::byte{0xFF}, {}, std::byte{0xFF}},
  };
  static constexpr auto kRgbaBytesV =
      std::bit_cast<std::array<std::byte, sizeof(kRgbaPixelsV)>>(kRgbaPixelsV);
  static constexpr auto kRgbyBitmapV = Bitmap{
      .bytes = kRgbaBytesV,
      .size = {2, 2},
  };
  auto texture_ci = Texture::CreateInfo{
      .device = *m_device_,
      .allocator = m_allocator_.get(),
      .queue_family = m_gpu_.queue_family,
      .command_block = create_command_block(),
      .bitmap = kRgbyBitmapV,
  };

  texture_ci.sampler.setMagFilter(vk::Filter::eNearest);
  m_texture_.emplace(std::move(texture_ci));
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
  return m_device_->allocateDescriptorSets(allocate_info);
}

void App::create_pipeline_layout() {
  static constexpr auto kSet0BindingsV = std::array{
      layout_binding(0, vk::DescriptorType::eUniformBuffer),
  };
  static constexpr auto kSet1BidningsV = std::array{
      layout_binding(0, vk::DescriptorType::eCombinedImageSampler),
  };
  static constexpr auto kSet2BidningsV = std::array{
      layout_binding(0, vk::DescriptorType::eStorageBuffer),
  };

  auto set_layout_cis = std::array<vk::DescriptorSetLayoutCreateInfo, 3>{};
  set_layout_cis[0].setBindings(kSet0BindingsV);
  set_layout_cis[1].setBindings(kSet1BidningsV);
  set_layout_cis[2].setBindings(kSet2BidningsV);

  for (auto const& set_layout_ci : set_layout_cis) {
    m_set_layouts_.push_back(
        m_device_->createDescriptorSetLayoutUnique(set_layout_ci));
    m_set_layout_views_.push_back(*m_set_layouts_.back());
  }

  auto push_constants_ranges = std::array<vk::PushConstantRange, 1>{};
  push_constants_ranges[0]
      .setOffset(0)
      .setSize(sizeof(PushConstants))
      .setStageFlags(vk::ShaderStageFlagBits::eVertex);

  for (auto const& push_constant_range : push_constants_ranges) {
    m_push_constant_ranges_.push_back(push_constant_range);
  }

  auto pipeline_layout_ci = vk::PipelineLayoutCreateInfo{};
  pipeline_layout_ci.setSetLayouts(m_set_layout_views_)
      .setPushConstantRanges(push_constants_ranges);
  m_pipeline_layout_ =
      m_device_->createPipelineLayoutUnique(pipeline_layout_ci);
}

void App::create_descriptor_pool() {
  static constexpr auto kPoolSizeV = std::array{
      vk::DescriptorPoolSize{vk::DescriptorType::eUniformBuffer,
                             kResourceBufferingV},
      vk::DescriptorPoolSize{vk::DescriptorType::eCombinedImageSampler,
                             kResourceBufferingV},
      vk::DescriptorPoolSize{vk::DescriptorType::eStorageBuffer,
                             kResourceBufferingV},
  };
  auto pool_ci = vk::DescriptorPoolCreateInfo{};
  pool_ci.setPoolSizes(kPoolSizeV).setMaxSets(16);
  m_descriptor_pool_ = m_device_->createDescriptorPoolUnique(pool_ci);
}

auto App::create_command_block() const -> CommandBlock {
  return CommandBlock{*m_device_, m_queue_, *m_cmd_block_pool_};
}

void App::create_cmd_block_pool() {
  auto command_pool_ci = vk::CommandPoolCreateInfo{};
  command_pool_ci.setQueueFamilyIndex(m_gpu_.queue_family)
      .setFlags(vk::CommandPoolCreateFlagBits::eTransient);
  m_cmd_block_pool_ = m_device_->createCommandPoolUnique(command_pool_ci);
}

void App::create_allocator() {
  m_allocator_ = vma::create_allocator(*m_instance_, m_gpu_.device, *m_device_);
}

void App::create_shader() {
  auto const vertex_spirv = to_spir_v(asset_path("shaders/mesh.vert"));
  auto const fragment_spirv = to_spir_v(asset_path("shaders/mesh.frag"));
  auto const shader_ci = ShaderProgram::CreateInfo{
      .device = *m_device_,
      .vertex_spirv = vertex_spirv,
      .fragment_spirv = fragment_spirv,
      .set_layouts = m_set_layout_views_,
      .push_constant_ranges = m_push_constant_ranges_,
  };
  m_shader_.emplace(shader_ci);
}

void App::create_render_sync() {
  auto command_pool_ci = vk::CommandPoolCreateInfo{};
  command_pool_ci.setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer)
      .setQueueFamilyIndex(m_gpu_.queue_family);
  m_render_cmd_pool_ = m_device_->createCommandPoolUnique(command_pool_ci);

  auto command_buffer_ai = vk::CommandBufferAllocateInfo{};
  command_buffer_ai.setCommandPool(*m_render_cmd_pool_)
      .setCommandBufferCount(static_cast<std::uint32_t>(kResourceBufferingV))
      .setLevel(vk::CommandBufferLevel::ePrimary);
  auto const command_buffers =
      m_device_->allocateCommandBuffers(command_buffer_ai);
  assert(command_buffers.size() == m_render_sync_.size());

  static constexpr auto kFenceCreateInfoV =
      vk::FenceCreateInfo{vk::FenceCreateFlagBits::eSignaled};
  for (auto [sync, command_buffer] :
       std::ranges::views::zip(m_render_sync_, command_buffers)) {
    sync.command_buffer = command_buffer;
    sync.draw = m_device_->createSemaphoreUnique({});
    sync.drawn = m_device_->createFenceUnique(kFenceCreateInfoV);
  }
}

void App::create_imgui() {
  auto const imgui_ci = DearImGui::CreateInfo{
      .window = m_window_.get(),
      .api_vesrion = kVkVersionV,
      .instance = *m_instance_,
      .physical_device = m_gpu_.device,
      .queue_family = m_gpu_.queue_family,
      .device = *m_device_,
      .queue = m_queue_,
      .color_format = m_swapchain_->get_format(),
      .samples = vk::SampleCountFlagBits::e1,
  };
  m_imgui_.emplace(imgui_ci);
}

void App::create_swapchain() {
  auto const size = glfw::framebuffer_size(m_window_.get());
  m_swapchain_.emplace(*m_device_, m_gpu_, m_allocator_.get(), *m_surface_,
                       size);
}

void App::create_depth_image() {}

void App::create_device() {
  auto queue_ci = vk::DeviceQueueCreateInfo{};
  static constexpr auto kQueuePrioritiesV = std::array{1.0F};
  queue_ci.setQueueFamilyIndex(m_gpu_.queue_family)
      .setQueueCount(1)
      .setQueuePriorities(kQueuePrioritiesV);

  auto enabled_features = vk::PhysicalDeviceFeatures{};
  enabled_features.setFillModeNonSolid(m_gpu_.features.fillModeNonSolid);
  enabled_features.setWideLines(m_gpu_.features.wideLines);
  enabled_features.setSamplerAnisotropy(m_gpu_.features.samplerAnisotropy);
  enabled_features.setSampleRateShading(m_gpu_.features.sampleRateShading);

  auto sync_feature = vk::PhysicalDeviceSynchronization2Features{vk::True};
  auto dynamic_redering_feature =
      vk::PhysicalDeviceDynamicRenderingFeatures{vk::True};

  auto shader_object_feature =
      vk::PhysicalDeviceShaderObjectFeaturesEXT{vk::True};

  auto buffer_device_address_feature = vk::PhysicalDeviceVulkan12Features{};
  buffer_device_address_feature.setScalarBlockLayout(vk::True);
  buffer_device_address_feature.setBufferDeviceAddress(vk::True);

  sync_feature.setPNext(&dynamic_redering_feature);
  dynamic_redering_feature.setPNext(&shader_object_feature);
  shader_object_feature.setPNext(&buffer_device_address_feature);

  auto device_ci = vk::DeviceCreateInfo{};
  static constexpr auto kExtensionsV = std::array{
      VK_KHR_SWAPCHAIN_EXTENSION_NAME,
      VK_EXT_SHADER_OBJECT_EXTENSION_NAME,
  };
  device_ci.setPEnabledExtensionNames(kExtensionsV)
      .setQueueCreateInfos(queue_ci)
      .setPEnabledFeatures(&enabled_features)
      .setPNext(&sync_feature);

  m_device_ = m_gpu_.device.createDeviceUnique(device_ci);
  VULKAN_HPP_DEFAULT_DISPATCHER.init(*m_device_);

  m_waiter_ = *m_device_;

  static constexpr std::uint32_t kQueueIndexV{0};
  m_queue_ = m_device_->getQueue(m_gpu_.queue_family, kQueueIndexV);
}

void App::select_gpu() {
  m_gpu_ = get_suitable_gpu(*m_instance_, *m_surface_);
  std::println(
      "Selected GPU: {}, type: {}",
      std::string_view{m_gpu_.properties.deviceName},
      m_gpu_.properties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu
          ? "DiscreteGpu"
          : "IntegratedGpu");
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

  for (const auto& mesh : asset_->meshes) {
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

    asset_.emplace(std::move(expected_asset.get()));
  }

  return true;
}

auto App::load_mesh(fastgltf::Mesh const& mesh) -> bool {
  assert(asset_.has_value());

  auto const& asset = *asset_;

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

    // // TANGENTS
    // {
    //   const auto* tangent_it = primitive.findAttribute("TANGENT");
    //   assert(tangent_it != primitive.attributes.end());
    //
    //   const auto& tangent_accessor =
    //   asset.accessors[tangent_it->accessorIndex];
    //   assert(tangent_accessor.bufferViewIndex.has_value());
    //
    //   fastgltf::iterateAccessorWithIndex<glm::vec4>(
    //       asset, tangent_accessor, [&](glm::vec4 tangent, std::size_t idx) {
    //         vertices[idx].tangent = tangent;
    //       });
    // }

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
    auto const vertex_ci = vma::BufferCreateInfo{
        .device = *m_device_,
        .allocator = m_allocator_.get(),
        .usage = vk::BufferUsageFlagBits::eShaderDeviceAddress,
        .queue_family = m_gpu_.queue_family,
    };
    auto vertex_buffer = vma::create_device_buffer(
        vertex_ci, create_command_block(),
        std::array<std::span<std::byte const>, 1>{to_byte_span(vertices)});
    out_primitive.vertex_buffer = std::move(vertex_buffer);

    // index buffer
    auto const index_ci = vma::BufferCreateInfo{
        .device = *m_device_,
        .allocator = m_allocator_.get(),
        .usage = vk::BufferUsageFlagBits::eIndexBuffer,
        .queue_family = m_gpu_.queue_family,
    };
    auto index_buffer = vma::create_device_buffer(
        index_ci, create_command_block(),
        std::array<std::span<std::byte const>, 1>{to_byte_span(indices)});
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
