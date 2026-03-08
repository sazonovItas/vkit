#pragma once

#include <filesystem>
#include <optional>

#include "bindless_set_manager.hpp"
#include "dear_imgui.hpp"
#include "descriptor_buffer.hpp"
#include "fastgltf/types.hpp"
#include "model.hpp"
#include "resource_buffering.hpp"
#include "shader_program.hpp"
#include "swapchain.hpp"
#include "texture.hpp"
#include "transform.hpp"
#include "vk_mem_alloc.hpp"
#include "vku/scoped/device_waiter.hpp"
#include "vulkan/gpu.hpp"
#include "window.hpp"

namespace fs = std::filesystem;

namespace lvk {
class App {
 public:
  void run();

 private:
  struct RenderSync {
    vk::UniqueSemaphore draw;
    vk::UniqueFence drawn;
    vk::CommandBuffer command_buffer;
  };

  void create_window();
  void create_instance();
  void create_surface();
  void create_device();
  void create_swapchain();
  void create_render_sync();
  void create_imgui();
  void create_bindless_set_manager();
  void create_descriptor_pool();
  void create_pipeline_layout();
  void create_shader();
  void create_shader_resources();
  void create_cmd_block_pool();
  void create_transfer_command_pool();
  void create_descriptor_sets();

  [[nodiscard]] auto asset_path(std::string_view uri) const -> fs::path;
  [[nodiscard]] auto allocate_sets() const -> std::vector<vk::DescriptorSet>;

  void main_loop();

  auto acquire_render_target() -> bool;
  auto begin_frame() -> vk::CommandBuffer;
  void transition_for_render(vk::CommandBuffer command_buffer) const;
  void render(vk::CommandBuffer command_buffer);
  void transition_for_present(vk::CommandBuffer command_buffer) const;
  void submit_and_present();

  void inspect();
  void update_view();
  void update_instances();
  void draw(vk::CommandBuffer command_buffer) const;
  void draw_mesh(vk::CommandBuffer command_buffer, std::size_t mesh_idx) const;
  void bind_descriptor_sets(vk::CommandBuffer command_buffer) const;

  void load_gltf();
  auto load_asset(fs::path const& path) -> bool;
  auto load_mesh(const fastgltf::Mesh& mesh) -> bool;
  auto load_texture(size_t idx, const fastgltf::Texture& texture) -> bool;
  auto load_material(const fastgltf::Material& material) -> bool;

  fs::path m_assets_dir_;

  glfw::Window m_window_;
  vk::UniqueInstance m_instance_;
  vk::UniqueDebugUtilsMessengerEXT m_debug_messenger_;
  vk::UniqueSurfaceKHR m_surface_;

  std::optional<vkit::vulkan::Gpu> m_gpu_;

  std::optional<Swapchain> m_swapchain_;

  vk::UniqueCommandPool m_render_cmd_pool_;
  vk::UniqueCommandPool m_cmd_block_pool_;
  vk::UniqueCommandPool transferCommnadPool_;
  Buffered<RenderSync> m_render_sync_{};
  std::size_t m_frame_index_{};

  std::optional<DearImGui> m_imgui_;

  vk::UniqueDescriptorPool m_descriptor_pool_;
  std::optional<BindlessSetManager> bindlessSetManager_;
  std::vector<vk::UniqueDescriptorSetLayout> m_set_layouts_;
  std::vector<vk::DescriptorSetLayout> m_set_layout_views_;
  std::vector<vk::PushConstantRange> m_push_constant_ranges_;
  vk::UniquePipelineLayout m_pipeline_layout_;

  std::optional<ShaderProgram> m_shader_;

  std::optional<fastgltf::Asset> m_asset_;
  std::vector<Mesh> meshes_;
  std::vector<Material> materials_;

  Transform m_transform_;

  std::optional<DescriptorBuffer<kResourceBufferingV>> m_view_ubo_;
  std::uint32_t m_curr_tex_idx_{0};
  Buffered<std::vector<vk::DescriptorSet>> m_descriptor_sets_{};

  vk::UniqueSampler m_default_sampler_;
  std::vector<std::string> m_texture_names_;
  std::vector<Texture2D> m_textures_;

  glm::ivec2 m_framebuffer_size_{};
  std::optional<RenderTarget> m_render_target_;

  bool m_wireframe_{false};
  float m_line_width_{1.0F};

  struct Camera {
    glm::vec3 position;
    glm::vec3 target;
    glm::vec3 up;
  };

  Camera m_camera_{
      .position = glm::vec3(0.0F, 2.0F, 2.0F),
      .target = glm::vec3(0.0F, 0.0F, 0.0F),
      .up = glm::vec3(0.0F, 1.0F, 0.0F),
  };

  vku::DeviceWaiter m_waiter_;
};
}  // namespace lvk
