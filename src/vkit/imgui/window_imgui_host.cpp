#include "vkit/imgui/window_imgui_host.hpp"

#include <imgui.h>
#include <imgui_internal.h>

namespace vkit::imgui {

WindowImguiHost::WindowImguiHost(ImguiRenderer& imguiRenderer,
                                 const std::string_view name,
                                 const std::string_view iniFilename)
    : ImguiHost{name, iniFilename}, imguiRenderer_{imguiRenderer} {
  auto& io = imguiContext_->IO;
  io.BackendPlatformUserData = this;
  io.BackendPlatformName = "vkit_window_host";
  io.BackendRendererName = "vkit_vulkan_renderer";
  io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;
  io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;

  setStyle();

  ImGui::SetCurrentContext(nullptr);
}

void WindowImguiHost::setDockLayoutCallback(DockLayoutCallback callback) {
  dockLayoutCallback_ = std::move(callback);
}

void WindowImguiHost::setStatusBarCallback(StatusBarCallback callback) {
  statusBarCallback_ = std::move(callback);
}

void WindowImguiHost::beginFrame(std::uint32_t width, std::uint32_t height,
                                 float dt) {
  if (width < 1 || height < 1) {
    isVisible_ = false;
    return;
  }
  isVisible_ = true;

  ImGui::SetCurrentContext(imguiContext_);

  auto& io = imguiContext_->IO;
  io.DisplaySize =
      ImVec2{static_cast<float>(width), static_cast<float>(height)};
  io.DeltaTime = dt > 0.0F ? dt : static_cast<float>(1.0 / 60.0);

  ImGui::NewFrame();

  if (beginCallback_) {
    beginCallback_(*this);
  }

  const auto status_bar_height =
      (statusBarCallback_) ? ImGui::GetFrameHeight() : 0.0F;
  const auto* viewport = ImGui::GetMainViewport();

  ImGui::SetNextWindowPos(viewport->Pos);
  ImGui::SetNextWindowSize(viewport->Size);
  ImGui::SetNextWindowViewport(viewport->ID);

  const auto window_flags =
      ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar |
      ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar |
      ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
      ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{0.0F, 0.0F});
  ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0F);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0F);
  ImGui::Begin("vkit_root_window", nullptr, window_flags);
  ImGui::PopStyleVar(3);

  rootDockId_ = ImGui::GetID("vkit_dockspace");

  if (dockLayoutCallback_) {
    const auto available_size = ImVec2{
        static_cast<float>(width),
        static_cast<float>(height),
    };
    dockLayoutCallback_(*this, available_size);
  }

  ImGui::DockSpace(rootDockId_, ImVec2{0.0F, -status_bar_height},
                   ImGuiDockNodeFlags_PassthruCentralNode);

  ImGui::SetCursorPos(ImVec2{0.0F, viewport->Size.y - status_bar_height});
  ImGui::BeginChild(
      "vkit_statusbar", ImVec2{viewport->Size.x, status_bar_height}, 0,
      ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
  if (statusBarCallback_) {
    statusBarCallback_(*this);
  }
  ImGui::EndChild();
}

void WindowImguiHost::endFrame() {
  if (!isVisible_) return;

  ImGui::SetCurrentContext(imguiContext_);

  ImGui::End();

  ImGui::EndFrame();
  ImGui::Render();
}

void WindowImguiHost::render(vk::CommandBuffer cb, std::size_t frameIndex) {
  if (!isVisible_) return;

  ImGui::SetCurrentContext(imguiContext_);
  auto* const draw_data = ImGui::GetDrawData();

  imguiRenderer_.renderDrawData(cb, draw_data, frameIndex);
}

void WindowImguiHost::setStyle() {
  ImGui::SetCurrentContext(imguiContext_);
  ImGuiStyle& style = ImGui::GetStyle();
  ImVec4* colors = style.Colors;

  const ImVec4 k_blender_canvas = ImVec4(0.08F, 0.08F, 0.08F, 1.00F);
  const ImVec4 k_blender_panel = ImVec4(0.16F, 0.16F, 0.16F, 1.00F);
  const ImVec4 k_blender_border = ImVec4(0.22F, 0.22F, 0.22F, 1.00F);

  const ImVec4 k_blender_bg = ImVec4(0.11F, 0.11F, 0.11F, 1.00F);
  const ImVec4 k_blender_frame = ImVec4(0.18F, 0.18F, 0.18F, 1.00F);
  const ImVec4 k_blender_frame_hover = ImVec4(0.24F, 0.24F, 0.24F, 1.00F);
  const ImVec4 k_blender_accent = ImVec4(0.34F, 0.58F, 0.90F, 1.00F);

  colors[ImGuiCol_Text] = ImVec4(0.85F, 0.85F, 0.85F, 1.00F);
  colors[ImGuiCol_TextDisabled] = ImVec4(0.50F, 0.50F, 0.50F, 1.00F);

  colors[ImGuiCol_WindowBg] = k_blender_panel;
  colors[ImGuiCol_ChildBg] = k_blender_bg;
  colors[ImGuiCol_PopupBg] = ImVec4(0.08F, 0.08F, 0.08F, 0.94F);

  colors[ImGuiCol_Border] = k_blender_border;
  colors[ImGuiCol_BorderShadow] = ImVec4(0.00F, 0.00F, 0.00F, 0.00F);

  colors[ImGuiCol_FrameBg] = k_blender_frame;
  colors[ImGuiCol_FrameBgHovered] = k_blender_frame_hover;
  colors[ImGuiCol_FrameBgActive] = k_blender_accent;

  colors[ImGuiCol_TitleBg] = k_blender_panel;
  colors[ImGuiCol_TitleBgActive] = k_blender_panel;
  colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00F, 0.00F, 0.00F, 0.51F);

  colors[ImGuiCol_MenuBarBg] = k_blender_panel;

  colors[ImGuiCol_Header] = k_blender_frame;
  colors[ImGuiCol_HeaderHovered] = k_blender_frame_hover;
  colors[ImGuiCol_HeaderActive] = k_blender_accent;

  colors[ImGuiCol_Tab] = k_blender_panel;
  colors[ImGuiCol_TabHovered] = k_blender_frame_hover;
  colors[ImGuiCol_TabActive] = k_blender_bg;
  colors[ImGuiCol_TabUnfocused] = k_blender_panel;
  colors[ImGuiCol_TabUnfocusedActive] = k_blender_bg;

  colors[ImGuiCol_DockingEmptyBg] = k_blender_canvas;

  style.WindowPadding = ImVec2(0.0F, 0.0F);
  style.FramePadding = ImVec2(2.0F, 2.0F);
  style.ItemSpacing = ImVec2(4.0F, 4.0F);
  style.ItemInnerSpacing = ImVec2(2.0F, 2.0F);

  style.WindowBorderSize = 1.0F;
  style.ChildBorderSize = 1.0F;
  style.FrameBorderSize = 0.0F;

  style.WindowRounding = 8.0F;
  style.ChildRounding = 6.0F;
  style.FrameRounding = 4.0F;
  style.PopupRounding = 6.0F;
  style.TabRounding = 4.0F;
  style.GrabRounding = 4.0F;
  style.ScrollbarRounding = 12.0F;
}

};  // namespace vkit::imgui
