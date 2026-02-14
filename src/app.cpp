#include "app.hpp"

#include <backends/imgui_impl_glfw.h>
#include <imgui.h>

#include <algorithm>
#include <cassert>
#include <chrono>
#include <fstream>
#include <optional>
#include <print>
#include <ranges>
#include <stdexcept>
#include <vector>

#include "command_block.hpp"
#include "dear_imgui.hpp"
#include "glm/ext/matrix_clip_space.hpp"
#include "glm/fwd.hpp"
#include "gpu.hpp"
#include "pipeline.hpp"
#include "resource_buffering.hpp"
#include "shader_program.hpp"
#include "vertex.hpp"
#include "vma.hpp"
#include "vulkan/vulkan.hpp"
#include "vulkan/vulkan_core.h"
#include "vulkan/vulkan_hpp_macros.hpp"
#include "window.hpp"

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE;

namespace {
constexpr auto kVkVersionV = VK_MAKE_VERSION(1, 3, 0);

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
  create_render_sync();
  create_imgui();
  create_descriptor_pool();
  create_pipeline_layout();
  create_alt_pipeline();
  create_shader();
  create_cmd_block_pool();

  create_shader_resources();
  create_descriptor_sets();

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

    static auto const kInspectTransform = [](Transform& out) {
      ImGui::DragFloat2("position", &out.position.x);
      ImGui::DragFloat("rotation", &out.rotation);
      ImGui::DragFloat2("scale", &out.scale.x, 0.1F);
    };

    ImGui::Separator();
    if (ImGui::TreeNode("View")) {
      kInspectTransform(m_view_transform_);
      ImGui::TreePop();
    }

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
  // command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics,
  // *m_pipeline_); command_buffer.setPolygonModeEXT(m_wireframe_ ?
  // vk::PolygonMode::eLine
  //                                               : vk::PolygonMode::eFill);
  // if (m_wireframe_) {
  //   command_buffer.setLineWidth(m_line_width_);
  // }
  //
  // auto viewport = vk::Viewport{};
  // viewport.setX(0.0F)
  //     .setY(static_cast<float>(m_render_target_->extent.height))
  //     .setWidth(static_cast<float>(m_render_target_->extent.width))
  //     .setHeight(-viewport.y);
  // command_buffer.setViewport(0, viewport);
  // command_buffer.setScissor(0, vk::Rect2D({}, m_render_target_->extent));

  m_shader_->bind(command_buffer, m_framebuffer_size_);

  bind_descriptor_sets(command_buffer);

  command_buffer.bindVertexBuffers(0, m_vbo_.get().buffer, vk::DeviceSize{});
  command_buffer.bindIndexBuffer(m_vbo_.get().buffer, 4 * sizeof(Vertex),
                                 vk::IndexType::eUint32);

  auto const instances = static_cast<std::uint32_t>(m_instances_.size());
  command_buffer.drawIndexed(6, instances, 0, 0, 0);
};

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
  auto barrier = m_swapchain_->base_barrier();

  barrier.setOldLayout(vk::ImageLayout::eUndefined)
      .setNewLayout(vk::ImageLayout::eAttachmentOptimal)
      .setSrcAccessMask(vk::AccessFlagBits2::eColorAttachmentRead |
                        vk::AccessFlagBits2::eColorAttachmentWrite)
      .setSrcStageMask(vk::PipelineStageFlagBits2::eColorAttachmentOutput)
      .setDstAccessMask(barrier.srcAccessMask)
      .setDstStageMask(barrier.srcStageMask);
  dependency_info.setImageMemoryBarriers(barrier);
  command_buffer.pipelineBarrier2(dependency_info);
}

void App::render(vk::CommandBuffer const command_buffer) {
  auto color_attachment = vk::RenderingAttachmentInfo{};
  color_attachment.setImageView(m_render_target_->image_view)
      .setImageLayout(vk::ImageLayout::eAttachmentOptimal)
      .setLoadOp(vk::AttachmentLoadOp::eClear)
      .setStoreOp(vk::AttachmentStoreOp::eStore)
      .setClearValue(vk::ClearColorValue{0.0F, 0.0F, 0.0F, 1.0F});

  auto rendering_info = vk::RenderingInfo{};
  auto const render_area = vk::Rect2D{vk::Offset2D{}, m_render_target_->extent};
  rendering_info.setRenderArea(render_area)
      .setColorAttachments(color_attachment)
      .setLayerCount(1)
      .setColorAttachments(color_attachment)
      .setPDepthAttachment(nullptr);

  update_view();
  update_instances();

  command_buffer.beginRendering(rendering_info);
  draw(command_buffer);
  command_buffer.endRendering();

  inspect();
  m_imgui_->end_frame();

  color_attachment.setLoadOp(vk::AttachmentLoadOp::eLoad);
  rendering_info.setColorAttachments(color_attachment)
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
  auto const mat_projection =
      glm::ortho(-half_size.x, half_size.x, -half_size.y, half_size.y);

  auto const mat_view = m_view_transform_.view_matrix();
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
  static constexpr auto kVerticesV = std::array{
      Vertex{.position = {-200.0F, -200.0F}, .uv = {0.0F, 1.0F}},
      Vertex{.position = {200.0F, -200.0F}, .uv = {1.0F, 1.0F}},
      Vertex{.position = {200.0F, 200.0F}, .uv = {1.0F, 0.0F}},
      Vertex{.position = {-200.0F, 200.0F}, .uv = {0.0F, 0.0F}},
  };
  static constexpr auto kIndicesV = std::array{
      0U, 1U, 2U, 2U, 3U, 0U,
  };
  static constexpr auto kVerticesBytesV = to_byte_array(kVerticesV);
  static constexpr auto kIndicesBytesV = to_byte_array(kIndicesV);
  static constexpr auto kTotalBytesV =
      std::array<std::span<std::byte const>, 2>{
          kVerticesBytesV,
          kIndicesBytesV,
      };

  auto const buffer_ci = vma::BufferCreateInfo{
      .allocator = m_allocator_.get(),
      .usage = vk::BufferUsageFlagBits::eVertexBuffer |
               vk::BufferUsageFlagBits::eIndexBuffer,
      .queue_family = m_gpu_.queue_family,
  };
  m_vbo_ = vma::create_device_buffer(buffer_ci, create_command_block(),
                                     kTotalBytesV);

  m_view_ubo_.emplace(m_allocator_.get(), m_gpu_.queue_family,
                      vk::BufferUsageFlagBits::eUniformBuffer);

  m_instance_ssbo_.emplace(m_allocator_.get(), m_gpu_.queue_family,
                           vk::BufferUsageFlagBits::eStorageBuffer);

  using Pixel = std::array<std::byte, 4>;
  using vma::Bitmap;

  static constexpr auto kRgbyPixelsV = std::array{
      Pixel{std::byte{0xFF}, {}, {}, std::byte{0xFF}},
      Pixel{std::byte{}, std::byte{0xFF}, {}, std::byte{0xFF}},
      Pixel{std::byte{}, {}, std::byte{0xFF}, std::byte{0xFF}},
      Pixel{std::byte{0xFF}, std::byte{0xFF}, {}, std::byte{0xFF}},
  };
  static constexpr auto kRgbyBytesV =
      std::bit_cast<std::array<std::byte, sizeof(kRgbyPixelsV)>>(kRgbyPixelsV);
  static constexpr auto kRgbyBitmapV = Bitmap{
      .bytes = kRgbyBytesV,
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

  auto pipeline_layout_ci = vk::PipelineLayoutCreateInfo{};
  pipeline_layout_ci.setSetLayouts(m_set_layout_views_);
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

void App::create_alt_pipeline() {
  auto const vertex_spirv = to_spir_v(asset_path("shader.vert"));
  auto const fragment_spirv = to_spir_v(asset_path("shader.frag"));
  if (vertex_spirv.empty() || fragment_spirv.empty()) {
    throw std::runtime_error{"failed to load shaders"};
  }

  auto pipeline_layout_ci = vk::PipelineLayoutCreateInfo{};
  pipeline_layout_ci.setSetLayouts({});
  m_alt_pipeline_layout_ =
      m_device_->createPipelineLayoutUnique(pipeline_layout_ci);

  auto const pipeline_builder_ci = PipelineBuilder::CreateInfo{
      .device = *m_device_,
      .samples = vk::SampleCountFlagBits::e1,
      .color_format = m_swapchain_->get_format(),
  };
  m_alt_pipeline_builder_.emplace(pipeline_builder_ci);

  auto vertex_ci = vk::ShaderModuleCreateInfo{};
  vertex_ci.setCode(vertex_spirv);
  auto fragment_ci = vk::ShaderModuleCreateInfo{};
  fragment_ci.setCode(fragment_spirv);

  auto const vertex_shader = m_device_->createShaderModuleUnique(vertex_ci);
  auto const fragment_shader = m_device_->createShaderModuleUnique(fragment_ci);
  auto const pipeline_state = PipelineState{
      .vertex_shader = *vertex_shader,
      .fragment_shader = *fragment_shader,
  };

  m_alt_pipeline_ =
      m_alt_pipeline_builder_->build(*m_alt_pipeline_layout_, pipeline_state);
}

void App::create_shader() {
  auto const vertex_spirv = to_spir_v(asset_path("shader.vert"));
  auto const fragment_spirv = to_spir_v(asset_path("shader.frag"));
  static constexpr auto kVertexInputV = ShaderVertexInput{
      .attributes = kVertexAttributesV,
      .bindings = kVertexBindingsV,
  };
  auto const shader_ci = ShaderProgram::CreateInfo{
      .device = *m_device_,
      .vertex_spirv = vertex_spirv,
      .fragment_spirv = fragment_spirv,
      .vertex_input = kVertexInputV,
      .set_layouts = m_set_layout_views_,
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
  m_swapchain_.emplace(*m_device_, m_gpu_, *m_surface_, size);
}

void App::create_device() {
  auto queue_ci = vk::DeviceQueueCreateInfo{};
  static constexpr auto kQueuePrioritiesV = std::array{1.0F};
  queue_ci.setQueueFamilyIndex(m_gpu_.queue_family)
      .setQueueCount(1)
      .setQueuePriorities(kQueuePrioritiesV);

  auto enabled_features = vk::PhysicalDeviceFeatures{};
  enabled_features.fillModeNonSolid = m_gpu_.features.fillModeNonSolid;
  enabled_features.wideLines = m_gpu_.features.wideLines;
  enabled_features.samplerAnisotropy = m_gpu_.features.samplerAnisotropy;
  enabled_features.sampleRateShading = m_gpu_.features.sampleRateShading;

  auto sync_feature = vk::PhysicalDeviceSynchronization2Features{vk::True};
  auto dynamic_redering_feature =
      vk::PhysicalDeviceDynamicRenderingFeatures{vk::True};
  sync_feature.setPNext(&dynamic_redering_feature);

  auto shader_object_feature =
      vk::PhysicalDeviceShaderObjectFeaturesEXT{vk::True};
  dynamic_redering_feature.setPNext(&shader_object_feature);

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
          ? "Discrete"
          : "Integrated");
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

  auto instance_ci = vk::InstanceCreateInfo{};

  auto const extensions = glfw::instance_extensions();

  static constexpr auto kLayersV = std::array{
      "VK_LAYER_KHRONOS_shader_object",
  };
  auto const layers = get_layers(kLayersV);

  instance_ci.setPApplicationInfo(&app_info)
      .setPEnabledExtensionNames(extensions)
      .setPEnabledLayerNames(kLayersV);

  m_instance_ = vk::createInstanceUnique(instance_ci);
  VULKAN_HPP_DEFAULT_DISPATCHER.init(*m_instance_);
}

void App::create_window() {
  m_window_ = glfw::create_window({1280, 720}, "learn vulkan");
}

auto App::asset_path(std::string_view uri) const -> fs::path {
  return m_assets_dir_ / uri;
}

}  // namespace lvk
