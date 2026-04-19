#include "vkit/window/window_event_handler.hpp"

#include <format>

namespace vkit::window {

auto c_str(const Keycode code) -> const char* {
  switch (code) {
    case kKeyUnknown:
      return "unknown";
    case kKeySpace:
      return "space";
    case kKeyApostrophe:
      return "apostrophe";
    case kKeyComma:
      return "comma";
    case kKeyMinus:
      return "minus";
    case kKeyPeriod:
      return "period";
    case kKeySlash:
      return "slash";
    case kKey0:
      return "0";
    case kKey1:
      return "1";
    case kKey2:
      return "2";
    case kKey3:
      return "3";
    case kKey4:
      return "4";
    case kKey5:
      return "5";
    case kKey6:
      return "6";
    case kKey7:
      return "7";
    case kKey8:
      return "8";
    case kKey9:
      return "9";
    case kKeySemicolon:
      return "semicolon";
    case kKeyEqual:
      return "equal";
    case kKeyA:
      return "a";
    case kKeyB:
      return "b";
    case kKeyC:
      return "c";
    case kKeyD:
      return "d";
    case kKeyE:
      return "e";
    case kKeyF:
      return "f";
    case kKeyG:
      return "g";
    case kKeyH:
      return "h";
    case kKeyI:
      return "i";
    case kKeyJ:
      return "j";
    case kKeyK:
      return "k";
    case kKeyL:
      return "l";
    case kKeyM:
      return "m";
    case kKeyN:
      return "n";
    case kKeyO:
      return "o";
    case kKeyP:
      return "p";
    case kKeyQ:
      return "q";
    case kKeyR:
      return "r";
    case kKeyS:
      return "s";
    case kKeyT:
      return "t";
    case kKeyU:
      return "u";
    case kKeyV:
      return "v";
    case kKeyW:
      return "w";
    case kKeyX:
      return "x";
    case kKeyY:
      return "y";
    case kKeyZ:
      return "z";
    case kKeyLeftBracket:
      return "left bracket";
    case kKeyBackslash:
      return "backslash";
    case kKeyRightBracket:
      return "right bracket";
    case kKeyGraveAccent:
      return "grave accent";
    case kKeyWorld1:
      return "world 1";
    case kKeyWorld2:
      return "world 2";
    case kKeyEscape:
      return "escape";
    case kKeyEnter:
      return "enter";
    case kKeyTab:
      return "tab";
    case kKeyBackspace:
      return "backspace";
    case kKeyInsert:
      return "insert";
    case kKeyDelete:
      return "delete";
    case kKeyRight:
      return "right";
    case kKeyLeft:
      return "left";
    case kKeyDown:
      return "down";
    case kKeyUp:
      return "up";
    case kKeyPageUp:
      return "page up";
    case kKeyPageDown:
      return "page down";
    case kKeyHome:
      return "home";
    case kKeyEnd:
      return "end";
    case kKeyCapsLock:
      return "caps lock";
    case kKeyScrollLock:
      return "scroll lock";
    case kKeyNumLock:
      return "num lock";
    case kKeyPrintScreen:
      return "print screen";
    case kKeyPause:
      return "pause";
    case kKeyF1:
      return "f1";
    case kKeyF2:
      return "f2";
    case kKeyF3:
      return "f3";
    case kKeyF4:
      return "f4";
    case kKeyF5:
      return "f5";
    case kKeyF6:
      return "f6";
    case kKeyF7:
      return "f7";
    case kKeyF8:
      return "f8";
    case kKeyF9:
      return "f9";
    case kKeyF10:
      return "f10";
    case kKeyF11:
      return "f11";
    case kKeyF12:
      return "f12";
    default:
      return "?";
  }
}

auto c_str(const MouseButton button) -> const char* {
  switch (button) {
    case kMouseButtonLeft:
      return "left";
    case kMouseButtonRight:
      return "right";
    case kMouseButtonMiddle:
      return "middle";
    case kMouseButtonWheel:
      return "wheel";
    case kMouseButtonX1:
      return "x1";
    case kMouseButtonX2:
      return "x2";
    default:
      return "?";
  }
}

InputEventHandler::~InputEventHandler() noexcept = default;

auto InputEventHandler::dispatchInputEvent(InputEvent& inputEvent) -> bool {
  switch (inputEvent.type) {
    case InputEventType::kNoEvent:
      break;
    case InputEventType::kKeyEvent:
      inputEvent.handled = onKeyEvent(inputEvent);
      break;
    case InputEventType::kTextEvent:
      inputEvent.handled = onTextEvent(inputEvent);
      break;
    case InputEventType::kCharEvent:
      inputEvent.handled = onCharEvent(inputEvent);
      break;
    case InputEventType::kWindowFocusEvent:
      inputEvent.handled = onWindowFocusEvent(inputEvent);
      break;
    case InputEventType::kCursorEnterEvent:
      inputEvent.handled = onCursorEnterEvent(inputEvent);
      break;
    case InputEventType::kMouseMoveEvent:
      inputEvent.handled = onMouseMoveEvent(inputEvent);
      break;
    case InputEventType::kMouseButtonEvent:
      inputEvent.handled = onMouseButtonEvent(inputEvent);
      break;
    case InputEventType::kMouseWheelEvent:
      inputEvent.handled = onMouseWheelEvent(inputEvent);
      break;
    case InputEventType::kWindowResizeEvent:
      inputEvent.handled = onWindowResizeEvent(inputEvent);
      break;
    case InputEventType::kWindowCloseEvent:
      inputEvent.handled = onWindowCloseEvent(inputEvent);
      break;
    case InputEventType::kWindowRefreshEvent:
      inputEvent.handled = onWindowRefreshEvent(inputEvent);
      break;
  }
  return inputEvent.handled;
}

auto InputEvent::describe() const -> std::string {
  switch (type) {
    case InputEventType::kNoEvent:
      return {};
    case InputEventType::kKeyEvent:
      return u.keyEvent.describe();
    case InputEventType::kTextEvent:
      return u.textEvent.describe();
    case InputEventType::kCharEvent:
      return u.charEvent.describe();
    case InputEventType::kWindowFocusEvent:
      return u.windowFocusEvent.describe();
    case InputEventType::kCursorEnterEvent:
      return u.cursorEnterEvent.describe();
    case InputEventType::kMouseMoveEvent:
      return u.mouseMoveEvent.describe();
    case InputEventType::kMouseButtonEvent:
      return u.mouseButtonEvent.describe();
    case InputEventType::kMouseWheelEvent:
      return u.mouseWheelEvent.describe();
    case InputEventType::kWindowResizeEvent:
      return u.windowResizeEvent.describe();
    case InputEventType::kWindowCloseEvent:
      return u.windowCloseEvent.describe();
    case InputEventType::kWindowRefreshEvent:
      return u.windowRefreshEvent.describe();
    default:
      return "?";
  }
}

auto KeyEvent::describe() const -> std::string {
  return std::format("KeyEvent keycode = {}, modifierMask = {:x}, pressed = {}",
                     keycode, modifierMask, pressed);
}

auto TextEvent::describe() const -> std::string {
  return std::format("TextEvent text = {}", utf8Text);
}

auto CharEvent::describe() const -> std::string {
  return std::format("CharEvent codepoint = {}", codepoint);
}

auto CursorEnterEvent::describe() const -> std::string {
  return std::format("CursorEnterEvent entered = {}", entered);
}

auto MouseMoveEvent::describe() const -> std::string {
  return std::format(
      "MouseMoveEvent x = {}, y = {}, dx = {}, dy = {}, modifierMask = {}", x,
      y, dx, dy, modifierMask);
}

auto MouseButtonEvent::describe() const -> std::string {
  return std::format(
      "MouseButtonEvent button = {}, pressed = {}, modifierMask = {}", button,
      pressed, modifierMask);
}

auto MouseWheelEvent::describe() const -> std::string {
  return std::format("MouseWheelEvent x = {}, y = {}, modifierMask = {}", x, y,
                     modifierMask);
}

auto WindowResizeEvent::describe() const -> std::string {
  return std::format("WindowResizeEvent width = {}, height = {}", width,
                     height);
}

auto WindowCloseEvent::describe() const -> std::string {
  return "WindowCloseEvent";
}

auto WindowRefreshEvent::describe() const -> std::string {
  return "WindowRefreshEvent";
}

auto WindowFocusEvent::describe() const -> std::string {
  return std::format("WindowFocusEvent focused = {}", focused);
}

};  // namespace vkit::window
