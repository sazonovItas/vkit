#include "vkit/imgui/window_imgui_host.hpp"

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

  const auto status_bar_height = ImGui::GetFrameHeight();
  const auto* viewport = ImGui::GetMainViewport();

  ImGui::SetNextWindowPos(viewport->Pos);
  ImGui::SetNextWindowSize(viewport->Size);
  ImGui::SetNextWindowViewport(viewport->ID);

  const auto window_flags =
      ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar |
      ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar |
      ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
      ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus |
      ImGuiWindowFlags_MenuBar;

  ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0F);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0F);

  ImGui::Begin("vkit_root_window", nullptr, window_flags);
  ImGui::PopStyleVar(2);

  rootDockId_ = ImGui::GetID("vkit_dockspace");

  if (dockLayoutCallback_) {
    auto const available_size = ImVec2{
        static_cast<float>(width),
        static_cast<float>(height) - status_bar_height,
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

};  // namespace vkit::imgui
