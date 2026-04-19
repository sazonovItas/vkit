#pragma once

#include <array>
#include <cstdint>
#include <vector>

#include "vkit/window/window_configuration.hpp"
#include "vkit/window/window_event_handler.hpp"

struct GLFWwindow;
struct GLFWcursor;

namespace vkit::window {

class Context {
 public:
  Context();
  ~Context();
};

using MouseCursor = std::int32_t;

constexpr MouseCursor kMouseCursorNone = -1;
constexpr MouseCursor kMouseCursorArrow = 0;
constexpr MouseCursor kMouseCursorTextInput = 1;
constexpr MouseCursor kMouseCursorResizeAll = 2;
constexpr MouseCursor kMouseCursorResizeNS = 3;
constexpr MouseCursor kMouseCursorResizeEW = 4;
constexpr MouseCursor kMouseCursorResizeNESW = 5;
constexpr MouseCursor kMouseCursorResizeNWSE = 6;
constexpr MouseCursor kMouseCursorHand = 7;
constexpr MouseCursor kMouseCursorNotAllowed = 8;
constexpr MouseCursor kMouseCursorCrosshair = 9;
constexpr MouseCursor kMouseCursorCount = 10;

class Window {
 public:
  Window(const Window&) = delete;
  Window& operator=(const Window&) = delete;

  Window(Window&& other) noexcept
      : window_(other.window_),
        cursors_(other.cursors_),
        currentCursor_(other.currentCursor_),
        width_(other.width_),
        height_(other.height_),
        modifiers_(other.modifiers_),
        eventWriteIndex_(other.eventWriteIndex_),
        events_(std::move(other.events_)) {
    other.window_ = nullptr;
    other.cursors_.fill(nullptr);
  }

  Window& operator=(Window&& other) noexcept {
    if (this == &other) return *this;

    window_ = other.window_;
    cursors_ = other.cursors_;
    currentCursor_ = other.currentCursor_;
    width_ = other.width_;
    height_ = other.height_;
    modifiers_ = other.modifiers_;
    eventWriteIndex_ = other.eventWriteIndex_;
    events_ = std::move(other.events_);

    other.window_ = nullptr;
    other.cursors_.fill(nullptr);

    return *this;
  }

  Window() = default;
  explicit Window(const WindowConfiguration& config);
  ~Window() noexcept;

  auto open(const WindowConfiguration& config) -> bool;
  void pollEvents();

  [[nodiscard]] auto shouldClose() const -> bool;

  [[nodiscard]] auto getWidth() const -> int;
  [[nodiscard]] auto getHeight() const -> int;

  [[nodiscard]] auto getGlfwWindow() const -> GLFWwindow*;

  [[nodiscard]] auto getInputEvents() -> std::vector<InputEvent>&;

  void setVisible(bool visible);
  void setCursor(MouseCursor cursor);

  void getCursorPosition(float& x, float& y);

  void handleKeyEvent(int64_t ts, int key, int scancode, int action, int mods);
  void handleCharEvent(int64_t ts, uint32_t codepoint);
  void handleMouseButtonEvent(int64_t ts, int button, int action, int mods);
  void handleMouseWheelEvent(int64_t ts, double x, double y);
  void handleMouseMove(int64_t ts, double x, double y);
  void handleWindowResizeEvent(int64_t ts, int width, int height);
  void handleWindowCloseEvent(int64_t ts);
  void handleWindowFocusEvent(int64_t ts, int focused);
  void handleCursorEnterEvent(int64_t ts, int entered);

  void injectInputEvent(const InputEvent& event);

  [[nodiscard]] auto getModifierMask() const -> KeyModifierMask;

  [[nodiscard]] auto createSurface(const vk::Instance& instance) const
      -> vk::UniqueSurfaceKHR;

  [[nodiscard]] static auto getVulkanExtensions() -> std::vector<const char*>;

 private:
  GLFWwindow* window_{nullptr};

  MouseCursor currentCursor_{kMouseCursorArrow};
  std::array<GLFWcursor*, kMouseCursorCount> cursors_{};

  WindowConfiguration config_;

  int width_{0};
  int height_{0};

  int modifiers_{0};

  int eventWriteIndex_{0};
  std::array<std::vector<InputEvent>, 2> events_;
};

}  // namespace vkit::window
