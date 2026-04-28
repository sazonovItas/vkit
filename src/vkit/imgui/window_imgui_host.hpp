#pragma once

#include <functional>

#include "vkit/imgui/imgui_host.hpp"
#include "vkit/imgui/imgui_renderer.hpp"

namespace vkit::imgui {

class WindowImguiHost : public ImguiHost {
 public:
  using DockLayoutCallback =
      std::function<void(WindowImguiHost& host, ImVec2 availableSize)>;
  using StatusBarCallback = std::function<void(WindowImguiHost& host)>;

  explicit WindowImguiHost(ImguiRenderer& imguiRenderer,
                           std::string_view name = "Vkit",
                           std::string_view iniFilename = "imgui.ini");
  ~WindowImguiHost() override = default;

  void setDockLayoutCallback(DockLayoutCallback callback);
  void setStatusBarCallback(StatusBarCallback callback);

  void beginFrame(std::uint32_t width, std::uint32_t height, float dt);
  void endFrame();

  void render(vk::CommandBuffer cb, std::size_t frameIndex);

 private:
  ImguiRenderer& imguiRenderer_;

  DockLayoutCallback dockLayoutCallback_;
  StatusBarCallback statusBarCallback_;

  bool isVisible_{false};
};

}  // namespace vkit::imgui
