#include "vkit/window/glfw_window.hpp"

#include <GLFW/glfw3.h>

#include <chrono>
#include <print>
#include <stdexcept>

#include "vkit/window/window_event_handler.hpp"

namespace vkit::window {

namespace {

int64_t nowNs() {
  return std::chrono::duration_cast<std::chrono::nanoseconds>(
             std::chrono::steady_clock::now().time_since_epoch())
      .count();
}

void keyCallback(GLFWwindow* w, int key, int scancode, int action, int mods) {
  auto* window = reinterpret_cast<Window*>(glfwGetWindowUserPointer(w));
  if (window) {
    window->handleKeyEvent(nowNs(), key, scancode, action, mods);
  }
}

void charCallback(GLFWwindow* w, unsigned int codepoint) {
  auto* window = reinterpret_cast<Window*>(glfwGetWindowUserPointer(w));
  if (window) {
    window->handleCharEvent(nowNs(), codepoint);
  }
}

void cursorPosCallback(GLFWwindow* w, double x, double y) {
  auto* window = reinterpret_cast<Window*>(glfwGetWindowUserPointer(w));
  if (window) {
    window->handleMouseMove(nowNs(), x, y);
  }
}

void mouseButtonCallback(GLFWwindow* w, int button, int action, int mods) {
  auto* window = reinterpret_cast<Window*>(glfwGetWindowUserPointer(w));
  if (window) {
    window->handleMouseButtonEvent(nowNs(), button, action, mods);
  }
}

void scrollCallback(GLFWwindow* w, double x, double y) {
  auto* window = reinterpret_cast<Window*>(glfwGetWindowUserPointer(w));
  if (window) {
    window->handleMouseWheelEvent(nowNs(), x, y);
  }
}

void resizeCallback(GLFWwindow* w, int width, int height) {
  auto* window = reinterpret_cast<Window*>(glfwGetWindowUserPointer(w));
  if (window) {
    window->handleWindowResizeEvent(nowNs(), width, height);
  }
}

void closeCallback(GLFWwindow* w) {
  auto* window = reinterpret_cast<Window*>(glfwGetWindowUserPointer(w));
  if (window) {
    window->handleWindowCloseEvent(nowNs());
  }
}

void focusCallback(GLFWwindow* w, int focused) {
  auto* window = reinterpret_cast<Window*>(glfwGetWindowUserPointer(w));
  if (window) {
    window->handleWindowFocusEvent(nowNs(), focused);
  }
}

void cursorEnterCallback(GLFWwindow* w, int entered) {
  auto* window = reinterpret_cast<Window*>(glfwGetWindowUserPointer(w));
  if (window) {
    window->handleCursorEnterEvent(nowNs(), entered);
  }
}

};  // namespace

Context::Context() {
  static auto const kOnError = [](int const code, char const* description) {
    std::println(stderr, "[GLFW] [ERROR] {}: {}", code, description);
  };
  glfwSetErrorCallback(kOnError);

  if (glfwInit() != GLFW_TRUE) {
    throw std::runtime_error{"Failed to init GLFW"};
  }

  if (glfwVulkanSupported() != GLFW_TRUE) {
    throw std::runtime_error{"GLFW doesn't support vulkan"};
  }
}

Context::~Context() { glfwTerminate(); }

Window::Window(const WindowConfiguration& config) {
  if (!open(config)) {
    throw std::runtime_error("[GLFW] Failed to create window");
  }
}

Window::~Window() noexcept {
  if (window_) {
    for (auto* c : cursors_) {
      if (c) {
        glfwDestroyCursor(c);
      }
    }

    glfwDestroyWindow(window_);
  }
}

auto Window::open(const WindowConfiguration& config) -> bool {
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_VISIBLE, config.show ? GLFW_TRUE : GLFW_FALSE);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

  GLFWmonitor* monitor = config.fullscreen ? glfwGetPrimaryMonitor() : nullptr;

  window_ = glfwCreateWindow(config.size.x, config.size.y, config.title.c_str(),
                             monitor, nullptr);

  if (!window_) {
    return false;
  }

  glfwSetWindowUserPointer(window_, this);

  glfwSetKeyCallback(window_, keyCallback);
  glfwSetCharCallback(window_, charCallback);
  glfwSetCursorPosCallback(window_, cursorPosCallback);
  glfwSetMouseButtonCallback(window_, mouseButtonCallback);
  glfwSetScrollCallback(window_, scrollCallback);
  glfwSetWindowSizeCallback(window_, resizeCallback);
  glfwSetWindowCloseCallback(window_, closeCallback);
  glfwSetWindowFocusCallback(window_, focusCallback);
  glfwSetCursorEnterCallback(window_, cursorEnterCallback);

  auto create_cursor = [&](int index, int shape) {
    cursors_[index] = glfwCreateStandardCursor(shape);
    if (!cursors_[index]) {
      cursors_[index] = cursors_[kMouseCursorArrow];
    }
  };

  create_cursor(kMouseCursorArrow, GLFW_ARROW_CURSOR);
  create_cursor(kMouseCursorTextInput, GLFW_IBEAM_CURSOR);
  create_cursor(kMouseCursorResizeNS, GLFW_VRESIZE_CURSOR);
  create_cursor(kMouseCursorResizeEW, GLFW_HRESIZE_CURSOR);
  create_cursor(kMouseCursorHand, GLFW_HAND_CURSOR);
  create_cursor(kMouseCursorCrosshair, GLFW_CROSSHAIR_CURSOR);

  glfwGetWindowSize(window_, &width_, &height_);

  return true;
}

void Window::pollEvents() {
  glfwPollEvents();

  auto& read = events_[1 - eventWriteIndex_];
  read.clear();

  eventWriteIndex_ = 1 - eventWriteIndex_;
}

auto Window::shouldClose() const -> bool {
  if (!window_) return true;
  return glfwWindowShouldClose(window_) == GLFW_TRUE;
}

auto Window::getWidth() const -> int { return width_; }

auto Window::getHeight() const -> int { return height_; }

auto Window::getGlfwWindow() const -> GLFWwindow* { return window_; }

auto Window::getUserData() const -> GLFWwindow* { return window_; }

void Window::setClipboardText(void* userData, const char* text) {
  auto* window = static_cast<Window*>(userData);
  if (window && window->getGlfwWindow()) {
    glfwSetClipboardString(window->getGlfwWindow(), text);
  }
}

auto Window::getClipboardText(void* userData) -> const char* {
  auto* window = static_cast<Window*>(userData);
  if (window && window->getGlfwWindow()) {
    return glfwGetClipboardString(window->getGlfwWindow());
  }
  return nullptr;
}

auto Window::getInputEvents() -> std::vector<InputEvent>& {
  return events_[1 - eventWriteIndex_];
}

void Window::setVisible(bool visible) {
  if (!window_) return;

  if (visible) {
    glfwShowWindow(window_);
  } else {
    glfwHideWindow(window_);
  }
}

void Window::setCursor(MouseCursor cursor) {
  if (!window_) return;

  if (cursor < 0 || cursor >= kMouseCursorCount) {
    cursor = kMouseCursorArrow;
  }

  if (currentCursor_ == cursor) return;

  glfwSetCursor(window_, cursors_[cursor]);
  currentCursor_ = cursor;
}

void Window::getCursorPosition(float& x, float& y) {
  double dx{0.0};
  double dy{0.0};
  glfwGetCursorPos(window_, &dx, &dy);
  x = static_cast<float>(dx);
  y = static_cast<float>(dy);
}

void Window::handleKeyEvent(int64_t ts, int key, int, int action, int mods) {
  modifiers_ = mods;

  events_[eventWriteIndex_].push_back(InputEvent{
      .type = InputEventType::kKeyEvent,
      .timestampNs = ts,
      .u =
          {
              .keyEvent =
                  KeyEvent{
                      .keycode = key,
                      .modifierMask = getModifierMask(),
                      .pressed =
                          (action == GLFW_PRESS || action == GLFW_REPEAT),
                  },
          },
  });
}

void Window::handleCharEvent(int64_t ts, uint32_t codepoint) {
  events_[eventWriteIndex_].push_back(InputEvent{
      .type = InputEventType::kCharEvent,
      .timestampNs = ts,
      .u =
          {
              .charEvent =
                  {
                      .codepoint = codepoint,
                  },
          },
  });
}

void Window::handleMouseButtonEvent(int64_t ts, int button, int action,
                                    int mods) {
  modifiers_ = mods;

  events_[eventWriteIndex_].push_back(InputEvent{
      .type = InputEventType::kMouseButtonEvent,
      .timestampNs = ts,
      .u =
          {
              .mouseButtonEvent =
                  {
                      .button = static_cast<std::uint32_t>(button),
                      .pressed = (action == GLFW_PRESS),
                      .modifierMask = getModifierMask(),
                  },
          },
  });
}

void Window::handleMouseWheelEvent(int64_t ts, double x, double y) {
  events_[eventWriteIndex_].push_back(InputEvent{
      .type = InputEventType::kMouseWheelEvent,
      .timestampNs = ts,
      .u =
          {
              .mouseWheelEvent =
                  {
                      .x = static_cast<float>(x),
                      .y = static_cast<float>(y),
                  },
          },
  });
}

void Window::handleMouseMove(int64_t ts, double x, double y) {
  events_[eventWriteIndex_].push_back(InputEvent{
      .type = InputEventType::kMouseMoveEvent,
      .timestampNs = ts,
      .u =
          {
              .mouseMoveEvent =
                  {
                      .x = static_cast<float>(x),
                      .y = static_cast<float>(y),
                  },
          },
  });
}

void Window::handleWindowResizeEvent(int64_t ts, int width, int height) {
  width_ = width;
  height_ = height;

  events_[eventWriteIndex_].push_back(InputEvent{
      .type = InputEventType::kWindowResizeEvent,
      .timestampNs = ts,
      .u =
          {
              .windowResizeEvent =
                  {
                      .width = width,
                      .height = height,
                  },
          },
  });
}

void Window::handleWindowCloseEvent(int64_t ts) {
  if (window_) {
    glfwSetWindowShouldClose(window_, GLFW_TRUE);
  }

  events_[eventWriteIndex_].push_back(InputEvent{
      .type = InputEventType::kWindowCloseEvent,
      .timestampNs = ts,
  });
}

void Window::handleWindowFocusEvent(int64_t ts, int focused) {
  events_[eventWriteIndex_].push_back(InputEvent{
      .type = InputEventType::kWindowFocusEvent,
      .timestampNs = ts,
      .u =
          {
              .windowFocusEvent =
                  {
                      .focused = (focused != 0),
                  },
          },
  });
}

void Window::handleCursorEnterEvent(int64_t ts, int entered) {
  events_[eventWriteIndex_].push_back(InputEvent{
      .type = InputEventType::kCursorEnterEvent,
      .timestampNs = ts,
      .u =
          {
              .cursorEnterEvent =
                  {
                      .entered = entered,
                  },
          },
  });
}

void Window::injectInputEvent(const InputEvent& event) {
  events_[eventWriteIndex_].push_back(event);
}

auto Window::getModifierMask() const -> KeyModifierMask {
  return static_cast<KeyModifierMask>(modifiers_);
}

auto Window::createSurface(const vk::Instance& instance) const
    -> vk::UniqueSurfaceKHR {
  VkSurfaceKHR ret{};
  auto const result = glfwCreateWindowSurface(instance, window_, nullptr, &ret);
  if (result != VK_SUCCESS || ret == VkSurfaceKHR{}) {
    throw std::runtime_error{"Failed to create vulkan surface"};
  }

  return vk::UniqueSurfaceKHR{ret, instance};
}

auto Window::getVulkanExtensions() -> std::vector<const char*> {
  uint32_t count = 0;
  const char** extensions = glfwGetRequiredInstanceExtensions(&count);

  std::vector<const char*> result;
  result.reserve(count);

  for (uint32_t i = 0; i < count; ++i) {
    result.push_back(extensions[i]);
  }

  return result;
}

};  // namespace vkit::window
