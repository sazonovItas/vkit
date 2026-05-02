#pragma once

#include <functional>

#include "vkit/imgui/imgui_host.hpp"
#include "vkit/imgui/imgui_renderer.hpp"
#include "vkit/window/window.hpp"

namespace vkit::imgui {

class WindowImguiHost : public ImguiHost {
 public:
  using DockLayoutCallback =
      std::function<void(WindowImguiHost& host, ImVec2 availableSize)>;
  using StatusBarCallback = std::function<void(WindowImguiHost& host)>;

  explicit WindowImguiHost(window::Window* window, ImguiRenderer& renderer,
                           std::string_view name,
                           std::string_view iniFilename = "");
  ~WindowImguiHost() override = default;

  void setDockLayoutCallback(DockLayoutCallback callback);
  void setStatusBarCallback(StatusBarCallback callback);

  void beginFrame(std::uint32_t width, std::uint32_t height, float dt);
  void endFrame();

  void render(std::uint32_t frameIndex, vk::CommandBuffer cb);

 protected:
  virtual void setStyle() {};

 private:
  ImguiRenderer& imguiRenderer_;

  DockLayoutCallback dockLayoutCallback_;
  StatusBarCallback statusBarCallback_;

  bool isVisible_{false};
};

}  // namespace vkit::imgui
