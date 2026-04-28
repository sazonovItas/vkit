#pragma once

#include <imgui.h>

#include <functional>
#include <string>

#include "vkit/window/window_event_handler.hpp"

namespace vkit::imgui {

class ImguiHost : public window::InputEventHandler {
 public:
  explicit ImguiHost(std::string_view name,
                     std::string_view iniFilename = "imgui.ini");
  ~ImguiHost() override;

  void beginFrame();
  void endFrame();

  void setBeginCallback(const std::function<void(ImguiHost&)>& callback);

  [[nodiscard]] auto name() const -> const std::string&;
  [[nodiscard]] auto imguiContext() const -> ImGuiContext*;
  [[nodiscard]] auto getRootDockId() const -> ImGuiID;
  [[nodiscard]] auto getMousePosition() const -> glm::vec2;
  [[nodiscard]] auto hasCursor() const -> bool;

  [[nodiscard]] auto wantCaptureKeyboard() const -> bool;
  [[nodiscard]] auto wantCaptureMouse() const -> bool;
  void updateInputRequest(bool requestKeyboard, bool requestMouse);

  auto onWindowFocusEvent(const window::InputEvent& event) -> bool override;
  auto onCursorEnterEvent(const window::InputEvent& event) -> bool override;
  auto onMouseMoveEvent(const window::InputEvent& event) -> bool override;
  auto onMouseButtonEvent(const window::InputEvent& event) -> bool override;
  auto onMouseWheelEvent(const window::InputEvent& event) -> bool override;
  auto onKeyEvent(const window::InputEvent& event) -> bool override;
  auto onTextEvent(const window::InputEvent& event) -> bool override;
  auto onCharEvent(const window::InputEvent& event) -> bool override;

 protected:
  std::string name_;
  std::string imguiIniPath_;
  std::function<void(ImguiHost&)> beginCallback_;

  ImGuiContext* imguiContext_{nullptr};
  ImGuiID rootDockId_{0};

  bool hasCursor_{false};
  bool requestKeyboard_{false};
  bool requestMouse_{false};
};

}  // namespace vkit::imgui
