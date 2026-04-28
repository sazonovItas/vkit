#include "imgui_host.hpp"

#include <imgui.h>
#include <imgui_internal.h>

namespace vkit::imgui {

namespace {

auto fromVkit(const window::Keycode keycode) -> ImGuiKey {
  switch (keycode) {
    case window::kKeyTab:
      return ImGuiKey_Tab;
    case window::kKeyLeft:
      return ImGuiKey_LeftArrow;
    case window::kKeyRight:
      return ImGuiKey_RightArrow;
    case window::kKeyUp:
      return ImGuiKey_UpArrow;
    case window::kKeyDown:
      return ImGuiKey_DownArrow;
    case window::kKeyPageUp:
      return ImGuiKey_PageUp;
    case window::kKeyPageDown:
      return ImGuiKey_PageDown;
    case window::kKeyHome:
      return ImGuiKey_Home;
    case window::kKeyEnd:
      return ImGuiKey_End;
    case window::kKeyInsert:
      return ImGuiKey_Insert;
    case window::kKeyDelete:
      return ImGuiKey_Delete;
    case window::kKeyBackspace:
      return ImGuiKey_Backspace;
    case window::kKeySpace:
      return ImGuiKey_Space;
    case window::kKeyEnter:
      return ImGuiKey_Enter;
    case window::kKeyEscape:
      return ImGuiKey_Escape;
    case window::kKeyLeftControl:
      return ImGuiKey_LeftCtrl;
    case window::kKeyLeftShift:
      return ImGuiKey_LeftShift;
    case window::kKeyLeftAlt:
      return ImGuiKey_LeftAlt;
    case window::kKeyLeftSuper:
      return ImGuiKey_LeftSuper;
    case window::kKeyRightControl:
      return ImGuiKey_RightCtrl;
    case window::kKeyRightShift:
      return ImGuiKey_RightShift;
    case window::kKeyRightAlt:
      return ImGuiKey_RightAlt;
    case window::kKeyRightSuper:
      return ImGuiKey_RightSuper;
    case window::kKeyMenu:
      return ImGuiKey_Menu;
    case window::kKey0:
      return ImGuiKey_0;
    case window::kKey1:
      return ImGuiKey_1;
    case window::kKey2:
      return ImGuiKey_2;
    case window::kKey3:
      return ImGuiKey_3;
    case window::kKey4:
      return ImGuiKey_4;
    case window::kKey5:
      return ImGuiKey_5;
    case window::kKey6:
      return ImGuiKey_6;
    case window::kKey7:
      return ImGuiKey_7;
    case window::kKey8:
      return ImGuiKey_8;
    case window::kKey9:
      return ImGuiKey_9;
    case window::kKeyA:
      return ImGuiKey_A;
    case window::kKeyB:
      return ImGuiKey_B;
    case window::kKeyC:
      return ImGuiKey_C;
    case window::kKeyD:
      return ImGuiKey_D;
    case window::kKeyE:
      return ImGuiKey_E;
    case window::kKeyF:
      return ImGuiKey_F;
    case window::kKeyG:
      return ImGuiKey_G;
    case window::kKeyH:
      return ImGuiKey_H;
    case window::kKeyI:
      return ImGuiKey_I;
    case window::kKeyJ:
      return ImGuiKey_J;
    case window::kKeyK:
      return ImGuiKey_K;
    case window::kKeyL:
      return ImGuiKey_L;
    case window::kKeyM:
      return ImGuiKey_M;
    case window::kKeyN:
      return ImGuiKey_N;
    case window::kKeyO:
      return ImGuiKey_O;
    case window::kKeyP:
      return ImGuiKey_P;
    case window::kKeyQ:
      return ImGuiKey_Q;
    case window::kKeyR:
      return ImGuiKey_R;
    case window::kKeyS:
      return ImGuiKey_S;
    case window::kKeyT:
      return ImGuiKey_T;
    case window::kKeyU:
      return ImGuiKey_U;
    case window::kKeyV:
      return ImGuiKey_V;
    case window::kKeyW:
      return ImGuiKey_W;
    case window::kKeyX:
      return ImGuiKey_X;
    case window::kKeyY:
      return ImGuiKey_Y;
    case window::kKeyZ:
      return ImGuiKey_Z;
    case window::kKeyF1:
      return ImGuiKey_F1;
    case window::kKeyF2:
      return ImGuiKey_F2;
    case window::kKeyF3:
      return ImGuiKey_F3;
    case window::kKeyF4:
      return ImGuiKey_F4;
    case window::kKeyF5:
      return ImGuiKey_F5;
    case window::kKeyF6:
      return ImGuiKey_F6;
    case window::kKeyF7:
      return ImGuiKey_F7;
    case window::kKeyF8:
      return ImGuiKey_F8;
    case window::kKeyF9:
      return ImGuiKey_F9;
    case window::kKeyF10:
      return ImGuiKey_F10;
    case window::kKeyF11:
      return ImGuiKey_F11;
    case window::kKeyF12:
      return ImGuiKey_F12;
    case window::kKeyApostrophe:
      return ImGuiKey_Apostrophe;
    case window::kKeyComma:
      return ImGuiKey_Comma;
    case window::kKeyMinus:
      return ImGuiKey_Minus;
    case window::kKeyPeriod:
      return ImGuiKey_Period;
    case window::kKeySlash:
      return ImGuiKey_Slash;
    case window::kKeySemicolon:
      return ImGuiKey_Semicolon;
    case window::kKeyEqual:
      return ImGuiKey_Equal;
    case window::kKeyLeftBracket:
      return ImGuiKey_LeftBracket;
    case window::kKeyBackslash:
      return ImGuiKey_Backslash;
    case window::kKeyRightBracket:
      return ImGuiKey_RightBracket;
    case window::kKeyGraveAccent:
      return ImGuiKey_GraveAccent;
    case window::kKeyCapsLock:
      return ImGuiKey_CapsLock;
    case window::kKeyScrollLock:
      return ImGuiKey_ScrollLock;
    case window::kKeyNumLock:
      return ImGuiKey_NumLock;
    case window::kKeyPrintScreen:
      return ImGuiKey_PrintScreen;
    case window::kKeyPause:
      return ImGuiKey_Pause;
    case window::kKeyKp0:
      return ImGuiKey_Keypad0;
    case window::kKeyKp1:
      return ImGuiKey_Keypad1;
    case window::kKeyKp2:
      return ImGuiKey_Keypad2;
    case window::kKeyKp3:
      return ImGuiKey_Keypad3;
    case window::kKeyKp4:
      return ImGuiKey_Keypad4;
    case window::kKeyKp5:
      return ImGuiKey_Keypad5;
    case window::kKeyKp6:
      return ImGuiKey_Keypad6;
    case window::kKeyKp7:
      return ImGuiKey_Keypad7;
    case window::kKeyKp8:
      return ImGuiKey_Keypad8;
    case window::kKeyKp9:
      return ImGuiKey_Keypad9;
    case window::kKeyKpDecimal:
      return ImGuiKey_KeypadDecimal;
    case window::kKeyKpDivide:
      return ImGuiKey_KeypadDivide;
    case window::kKeyKpMultiply:
      return ImGuiKey_KeypadMultiply;
    case window::kKeyKpSubtract:
      return ImGuiKey_KeypadSubtract;
    case window::kKeyKpAdd:
      return ImGuiKey_KeypadAdd;
    case window::kKeyKpEnter:
      return ImGuiKey_KeypadEnter;
    case window::kKeyKpEqual:
      return ImGuiKey_KeypadEqual;
    default:
      return ImGuiKey_None;
  }
}

auto fromVkit(const window::MouseButton button) -> int {
  switch (button) {
    case window::kMouseButtonLeft:
      return ImGuiMouseButton_Left;
    case window::kMouseButtonRight:
      return ImGuiMouseButton_Right;
    case window::kMouseButtonMiddle:
      return ImGuiMouseButton_Middle;
    case window::kMouseButtonX1:
      return 3;
    case window::kMouseButtonX2:
      return 4;
    default:
      return -1;
  }
}

void updateKeyModifiers(ImGuiIO& io, const uint32_t modifierMask) {
  io.AddKeyEvent(ImGuiMod_Ctrl,
                 (modifierMask & window::kKeyModifierBitCtrl) != 0);
  io.AddKeyEvent(ImGuiMod_Shift,
                 (modifierMask & window::kKeyModifierBitShift) != 0);
  io.AddKeyEvent(ImGuiMod_Alt,
                 (modifierMask & window::kKeyModifierBitMenu) != 0);
  io.AddKeyEvent(ImGuiMod_Super,
                 (modifierMask & window::kKeyModifierBitSuper) != 0);
}

};  // namespace

ImguiHost::ImguiHost(const std::string_view name,
                     const std::string_view iniFilename)
    : name_{name}, imguiIniPath_{iniFilename} {
  IMGUI_CHECKVERSION();
  imguiContext_ = ImGui::CreateContext();
  ImGui::SetCurrentContext(imguiContext_);

  auto& io = imguiContext_->IO;
  io.IniFilename = imguiIniPath_.empty() ? nullptr : imguiIniPath_.c_str();
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
}

ImguiHost::~ImguiHost() {
  if (imguiContext_) {
    ImGui::SetCurrentContext(imguiContext_);

    ImGui::GetIO().BackendPlatformUserData = nullptr;
    ImGui::GetIO().BackendRendererUserData = nullptr;

    ImGui::DestroyContext(imguiContext_);
    imguiContext_ = nullptr;
  }
}

void ImguiHost::beginFrame() {
  ImGui::SetCurrentContext(imguiContext_);
  if (beginCallback_) {
    beginCallback_(*this);
  }
  ImGui::NewFrame();
}

void ImguiHost::endFrame() {
  ImGui::SetCurrentContext(imguiContext_);
  ImGui::Render();
}

void ImguiHost::setBeginCallback(
    const std::function<void(ImguiHost&)>& callback) {
  beginCallback_ = callback;
}

auto ImguiHost::name() const -> const std::string& { return name_; }

auto ImguiHost::imguiContext() const -> ImGuiContext* { return imguiContext_; }

auto ImguiHost::getRootDockId() const -> ImGuiID { return rootDockId_; }

auto ImguiHost::hasCursor() const -> bool { return hasCursor_; }

auto ImguiHost::getMousePosition() const -> glm::vec2 {
  if (!imguiContext_) return glm::vec2{0.0F};
  auto& io = imguiContext_->IO;
  return glm::vec2{io.MousePos.x, io.MousePos.y};
}

auto ImguiHost::wantCaptureKeyboard() const -> bool {
  if (!imguiContext_) return false;
  auto& io = imguiContext_->IO;
  return io.WantCaptureKeyboard && !requestKeyboard_;
}

auto ImguiHost::wantCaptureMouse() const -> bool {
  if (!imguiContext_) return false;
  auto& io = imguiContext_->IO;
  return io.WantCaptureMouse && !requestMouse_;
}

void ImguiHost::updateInputRequest(bool requestKeyboard, bool requestMouse) {
  requestKeyboard_ = requestKeyboard;
  requestMouse_ = requestMouse;
}

auto ImguiHost::onKeyEvent(const window::InputEvent& event) -> bool {
  if (!imguiContext_) return false;
  auto& io = imguiContext_->IO;

  updateKeyModifiers(io, event.u.keyEvent.modifierMask);
  io.AddKeyEvent(fromVkit(event.u.keyEvent.keycode), event.u.keyEvent.pressed);

  return wantCaptureKeyboard();
}

auto ImguiHost::onTextEvent(const window::InputEvent& event) -> bool {
  if (!imguiContext_) return false;
  auto& io = imguiContext_->IO;

  io.AddInputCharactersUTF8(event.u.textEvent.utf8Text);
  return wantCaptureKeyboard();
}

auto ImguiHost::onCharEvent(const window::InputEvent& event) -> bool {
  if (!imguiContext_) return false;
  auto& io = imguiContext_->IO;

  io.AddInputCharacter(event.u.charEvent.codepoint);
  return wantCaptureKeyboard();
}

auto ImguiHost::onCursorEnterEvent(const window::InputEvent& event) -> bool {
  if (!imguiContext_) return false;
  auto& io = imguiContext_->IO;

  auto const entered = event.u.cursorEnterEvent.entered != 0;
  hasCursor_ = entered;

  if (!hasCursor_) {
    io.AddMousePosEvent(-FLT_MAX, -FLT_MAX);
  }
  return true;
}

auto ImguiHost::onWindowFocusEvent(const window::InputEvent& event) -> bool {
  if (!imguiContext_) return false;
  auto& io = imguiContext_->IO;

  io.AddFocusEvent(event.u.windowFocusEvent.focused);
  return true;
}

auto ImguiHost::onMouseMoveEvent(const window::InputEvent& event) -> bool {
  if (!imguiContext_) return false;
  auto& io = imguiContext_->IO;

  io.AddMousePosEvent(event.u.mouseMoveEvent.x, event.u.mouseMoveEvent.y);
  return wantCaptureMouse();
}

auto ImguiHost::onMouseButtonEvent(const window::InputEvent& event) -> bool {
  if (!imguiContext_) return false;

  const auto imgui_mouse_button = fromVkit(event.u.mouseButtonEvent.button);
  if (imgui_mouse_button < 0) {
    return false;
  }

  auto& io = imguiContext_->IO;
  io.AddMouseButtonEvent(imgui_mouse_button, event.u.mouseButtonEvent.pressed);

  return wantCaptureMouse();
}

auto ImguiHost::onMouseWheelEvent(const window::InputEvent& event) -> bool {
  if (!imguiContext_) return false;
  auto& io = imguiContext_->IO;

  io.AddMouseWheelEvent(event.u.mouseWheelEvent.x, event.u.mouseWheelEvent.y);
  return wantCaptureMouse();
}

};  // namespace vkit::imgui
