#include "dear_imgui.hpp"

#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>
#include <imgui.h>

#include <stdexcept>

#include "glm/gtc/color_space.hpp"
#include "resource_buffering.hpp"

namespace vkit {
DearImGui::DearImGui(const CreateInfo& createInfo) {
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();

  static auto const kLoadVkFunc = [](char const* name, void* user_data) {
    return VULKAN_HPP_DEFAULT_DISPATCHER.vkGetInstanceProcAddr(
        *static_cast<vk::Instance*>(user_data), name);
  };

  auto instance = createInfo.instance;
  ImGui_ImplVulkan_LoadFunctions(createInfo.apiVersion, kLoadVkFunc, &instance);

  if (!ImGui_ImplGlfw_InitForVulkan(createInfo.window, true)) {
    throw std::runtime_error{"failed to init dear imgui"};
  }

  auto init_info = ImGui_ImplVulkan_InitInfo{};
  init_info.ApiVersion = createInfo.apiVersion;
  init_info.Instance = createInfo.instance;
  init_info.PhysicalDevice = createInfo.physicalDevice;
  init_info.Device = createInfo.device;
  init_info.QueueFamily = createInfo.queueFamily;
  init_info.Queue = createInfo.queue;
  init_info.MinImageCount = kResourceBufferingV;
  init_info.ImageCount = static_cast<std::uint32_t>(kResourceBufferingV);
  init_info.DescriptorPoolSize = 8;
  init_info.PipelineInfoMain.MSAASamples =
      static_cast<VkSampleCountFlagBits>(createInfo.samples);
  auto pipeline_rendering_ci = vk::PipelineRenderingCreateInfo{};
  pipeline_rendering_ci.setColorAttachmentCount(1).setColorAttachmentFormats(
      createInfo.colorFormat);
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

  device_ = vku::Scoped<vk::Device, Deleter>{createInfo.device};
}

void DearImGui::newFrame() {
  if (state_ == State::kBegun) {
    endFrame();
  }
  ImGui_ImplGlfw_NewFrame();
  ImGui_ImplVulkan_NewFrame();
  ImGui::NewFrame();
  state_ = State::kBegun;
}

void DearImGui::endFrame() {
  if (state_ == State::kEnded) {
    return;
  }
  ImGui::Render();
  state_ = State::kEnded;
}

void DearImGui::render(vk::CommandBuffer cb) const {
  auto* data = ImGui::GetDrawData();
  if (data == nullptr) {
    return;
  }

  ImGui_ImplVulkan_RenderDrawData(data, cb);
}

void DearImGui::Deleter::operator()(vk::Device const device) const {
  device.waitIdle();
  ImGui_ImplVulkan_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
}
};  // namespace vkit
