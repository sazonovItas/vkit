#include "dear_imgui.hpp"

#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>
#include <imgui.h>

#include <stdexcept>

#include "glm/gtc/color_space.hpp"
#include "resource_buffering.hpp"
#include "vulkan/vulkan.hpp"
#include "vulkan/vulkan_core.h"

namespace lvk {
DearImGui::DearImGui(CreateInfo const& create_info) {
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();

  static auto const kLoadVkFunc = [](char const* name, void* user_data) {
    return VULKAN_HPP_DEFAULT_DISPATCHER.vkGetInstanceProcAddr(
        *static_cast<vk::Instance*>(user_data), name);
  };

  auto instance = create_info.instance;
  ImGui_ImplVulkan_LoadFunctions(create_info.api_vesrion, kLoadVkFunc,
                                 &instance);

  if (!ImGui_ImplGlfw_InitForVulkan(create_info.window, true)) {
    throw std::runtime_error{"failed to init dear imgui"};
  }

  auto init_info = ImGui_ImplVulkan_InitInfo{};
  init_info.ApiVersion = create_info.api_vesrion;
  init_info.Instance = create_info.instance;
  init_info.PhysicalDevice = create_info.physical_device;
  init_info.Device = create_info.device;
  init_info.QueueFamily = create_info.queue_family;
  init_info.Queue = create_info.queue;
  init_info.MinImageCount = 3;
  init_info.ImageCount = static_cast<std::uint32_t>(kResourceBufferingV);
  init_info.DescriptorPoolSize = 8;
  init_info.PipelineInfoMain.MSAASamples =
      static_cast<VkSampleCountFlagBits>(create_info.samples);
  auto pipeline_rendering_ci = vk::PipelineRenderingCreateInfo{};
  pipeline_rendering_ci.setColorAttachmentCount(1).setColorAttachmentFormats(
      create_info.color_format);
  init_info.PipelineInfoMain.PipelineRenderingCreateInfo =
      pipeline_rendering_ci;
  init_info.UseDynamicRendering = true;
  if (!ImGui_ImplVulkan_Init(&init_info)) {
    throw std::runtime_error{"failed to init dear imgui"};
  }

  ImGui::StyleColorsDark();
  for (auto& color : ImGui::GetStyle().Colors) {
    auto const linear =
        glm::convertSRGBToLinear(glm::vec4{color.x, color.y, color.z, color.w});
    color = ImVec4{linear.x, linear.y, linear.z, linear.w};
  }
  ImGui::GetStyle().Colors[ImGuiCol_WindowBg].w = 0.99F;

  m_device_ = vkit::util::Scoped<vk::Device, Deleter>{create_info.device};
}

void DearImGui::new_frame() {
  if (m_state_ == State::kBegun) {
    end_frame();
  }
  ImGui_ImplGlfw_NewFrame();
  ImGui_ImplVulkan_NewFrame();
  ImGui::NewFrame();
  m_state_ = State::kBegun;
}

void DearImGui::end_frame() {
  if (m_state_ == State::kEnded) {
    return;
  }
  ImGui::Render();
  m_state_ = State::kEnded;
}

void DearImGui::render(vk::CommandBuffer const command_buffer) const {
  auto* data = ImGui::GetDrawData();
  if (data == nullptr) {
    return;
  }
  ImGui_ImplVulkan_RenderDrawData(data, command_buffer);
}

void DearImGui::Deleter::operator()(vk::Device const device) const {
  device.waitIdle();
  ImGui_ImplVulkan_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
}
};  // namespace lvk
