#pragma once
#include <filesystem>
#include <optional>

#include "command_block.hpp"
#include "dear_imgui.hpp"
#include "descriptor_buffer.hpp"
#include "fastgltf/types.hpp"
#include "gpu.hpp"
#include "model.hpp"
#include "resource_buffering.hpp"
#include "scoped_waiter.hpp"
#include "shader_program.hpp"
#include "swapchain.hpp"
#include "texture.hpp"
#include "transform.hpp"
#include "vma.hpp"
#include "vulkan/vulkan.hpp"
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
  void select_gpu();
  void create_device();
  void create_allocator();
  void create_swapchain();
  void create_depth_image();
  void create_render_sync();
  void create_imgui();
  void create_descriptor_pool();
  void create_pipeline_layout();
  void create_shader();
  void create_shader_resources();
  void create_cmd_block_pool();
  void create_descriptor_sets();

  [[nodiscard]] auto asset_path(std::string_view uri) const -> fs::path;
  [[nodiscard]] auto create_command_block() const -> CommandBlock;
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
  void draw_mesh(vk::CommandBuffer command_buffer, Mesh const& mesh) const;
  void bind_descriptor_sets(vk::CommandBuffer command_buffer) const;

  void load_gltf();
  auto load_asset(fs::path const& path) -> bool;
  auto load_mesh(fastgltf::Mesh const& mesh) -> bool;

  fs::path m_assets_dir_;

  glfw::Window m_window_;
  vk::UniqueInstance m_instance_;
  vk::UniqueDebugUtilsMessengerEXT m_debug_messenger_;
  vk::UniqueSurfaceKHR m_surface_;

  Gpu m_gpu_{};
  vk::UniqueDevice m_device_;
  vk::Queue m_queue_;
  vma::Allocator m_allocator_;

  std::optional<Swapchain> m_swapchain_;

  vma::Image m_depth_image_;
  vk::UniqueImageView m_depth_image_view_;

  vk::UniqueCommandPool m_render_cmd_pool_;
  vk::UniqueCommandPool m_cmd_block_pool_;
  Buffered<RenderSync> m_render_sync_{};
  std::size_t m_frame_index_{};

  std::optional<DearImGui> m_imgui_;

  vk::UniqueDescriptorPool m_descriptor_pool_;
  std::vector<vk::UniqueDescriptorSetLayout> m_set_layouts_;
  std::vector<vk::DescriptorSetLayout> m_set_layout_views_;
  std::vector<vk::PushConstantRange> m_push_constant_ranges_;
  vk::UniquePipelineLayout m_pipeline_layout_;

  std::optional<ShaderProgram> m_shader_;

  std::optional<fastgltf::Asset> asset_;
  std::vector<Mesh> meshes_;

  std::optional<DescriptorBuffer> m_view_ubo_;
  std::optional<Texture> m_texture_;
  std::optional<DescriptorBuffer> m_instance_ssbo_;
  Buffered<std::vector<vk::DescriptorSet>> m_descriptor_sets_{};

  glm::ivec2 m_framebuffer_size_{};
  std::optional<RenderTarget> m_render_target_;

  bool m_wireframe_{false};
  float m_line_width_{1.0F};

  std::vector<glm::mat4> m_instance_data_;

  struct Camera {
    glm::vec3 position;
    glm::vec3 target;
    glm::vec3 up;
  };

  Camera m_camera_{
      .position = glm::vec3(0.0F, 2.0F, 1.0F),
      .target = glm::vec3(0.0F, 0.0F, 0.0F),
      .up = glm::vec3(0.0F, 1.0F, 0.0F),
  };
  std::array<Transform, 1> m_instances_{
      Transform{
          .rotation = glm::vec3(0.0F, 45.0F, 0.0F),
      },
  };

  ScopedWaiter m_waiter_;
};
}  // namespace lvk
