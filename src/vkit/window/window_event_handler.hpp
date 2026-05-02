#pragma once

#include <cstdint>
#include <string>

namespace vkit::window {

using Keycode = std::int32_t;

constexpr Keycode kKeyUnknown = -1;
constexpr Keycode kKeySpace = 32;
constexpr Keycode kKeyApostrophe = 39;
constexpr Keycode kKeyComma = 44;
constexpr Keycode kKeyMinus = 45;
constexpr Keycode kKeyPeriod = 46;
constexpr Keycode kKeySlash = 47;
constexpr Keycode kKey0 = 48;
constexpr Keycode kKey1 = 49;
constexpr Keycode kKey2 = 50;
constexpr Keycode kKey3 = 51;
constexpr Keycode kKey4 = 52;
constexpr Keycode kKey5 = 53;
constexpr Keycode kKey6 = 54;
constexpr Keycode kKey7 = 55;
constexpr Keycode kKey8 = 56;
constexpr Keycode kKey9 = 57;
constexpr Keycode kKeySemicolon = 59;
constexpr Keycode kKeyEqual = 61;
constexpr Keycode kKeyA = 65;
constexpr Keycode kKeyB = 66;
constexpr Keycode kKeyC = 67;
constexpr Keycode kKeyD = 68;
constexpr Keycode kKeyE = 69;
constexpr Keycode kKeyF = 70;
constexpr Keycode kKeyG = 71;
constexpr Keycode kKeyH = 72;
constexpr Keycode kKeyI = 73;
constexpr Keycode kKeyJ = 74;
constexpr Keycode kKeyK = 75;
constexpr Keycode kKeyL = 76;
constexpr Keycode kKeyM = 77;
constexpr Keycode kKeyN = 78;
constexpr Keycode kKeyO = 79;
constexpr Keycode kKeyP = 80;
constexpr Keycode kKeyQ = 81;
constexpr Keycode kKeyR = 82;
constexpr Keycode kKeyS = 83;
constexpr Keycode kKeyT = 84;
constexpr Keycode kKeyU = 85;
constexpr Keycode kKeyV = 86;
constexpr Keycode kKeyW = 87;
constexpr Keycode kKeyX = 88;
constexpr Keycode kKeyY = 89;
constexpr Keycode kKeyZ = 90;
constexpr Keycode kKeyLeftBracket = 91;
constexpr Keycode kKeyBackslash = 92;
constexpr Keycode kKeyRightBracket = 93;
constexpr Keycode kKeyGraveAccent = 96;
constexpr Keycode kKeyWorld1 = 161;
constexpr Keycode kKeyWorld2 = 162;
constexpr Keycode kKeyEscape = 256;
constexpr Keycode kKeyEnter = 257;
constexpr Keycode kKeyTab = 258;
constexpr Keycode kKeyBackspace = 259;
constexpr Keycode kKeyInsert = 260;
constexpr Keycode kKeyDelete = 261;
constexpr Keycode kKeyRight = 262;
constexpr Keycode kKeyLeft = 263;
constexpr Keycode kKeyDown = 264;
constexpr Keycode kKeyUp = 265;
constexpr Keycode kKeyPageUp = 266;
constexpr Keycode kKeyPageDown = 267;
constexpr Keycode kKeyHome = 268;
constexpr Keycode kKeyEnd = 269;
constexpr Keycode kKeyCapsLock = 280;
constexpr Keycode kKeyScrollLock = 281;
constexpr Keycode kKeyNumLock = 282;
constexpr Keycode kKeyPrintScreen = 283;
constexpr Keycode kKeyPause = 284;
constexpr Keycode kKeyF1 = 290;
constexpr Keycode kKeyF2 = 291;
constexpr Keycode kKeyF3 = 292;
constexpr Keycode kKeyF4 = 293;
constexpr Keycode kKeyF5 = 294;
constexpr Keycode kKeyF6 = 295;
constexpr Keycode kKeyF7 = 296;
constexpr Keycode kKeyF8 = 297;
constexpr Keycode kKeyF9 = 298;
constexpr Keycode kKeyF10 = 299;
constexpr Keycode kKeyF11 = 300;
constexpr Keycode kKeyF12 = 301;
constexpr Keycode kKeyF13 = 302;
constexpr Keycode kKeyF14 = 303;
constexpr Keycode kKeyF15 = 304;
constexpr Keycode kKeyF16 = 305;
constexpr Keycode kKeyF17 = 306;
constexpr Keycode kKeyF18 = 307;
constexpr Keycode kKeyF19 = 308;
constexpr Keycode kKeyF20 = 309;
constexpr Keycode kKeyF21 = 310;
constexpr Keycode kKeyF22 = 311;
constexpr Keycode kKeyF23 = 312;
constexpr Keycode kKeyF24 = 313;
constexpr Keycode kKeyF25 = 314;
constexpr Keycode kKeyKp0 = 320;
constexpr Keycode kKeyKp1 = 321;
constexpr Keycode kKeyKp2 = 322;
constexpr Keycode kKeyKp3 = 323;
constexpr Keycode kKeyKp4 = 324;
constexpr Keycode kKeyKp5 = 325;
constexpr Keycode kKeyKp6 = 326;
constexpr Keycode kKeyKp7 = 327;
constexpr Keycode kKeyKp8 = 328;
constexpr Keycode kKeyKp9 = 329;
constexpr Keycode kKeyKpDecimal = 330;
constexpr Keycode kKeyKpDivide = 331;
constexpr Keycode kKeyKpMultiply = 332;
constexpr Keycode kKeyKpSubtract = 333;
constexpr Keycode kKeyKpAdd = 334;
constexpr Keycode kKeyKpEnter = 335;
constexpr Keycode kKeyKpEqual = 336;
constexpr Keycode kKeyLeftShift = 340;
constexpr Keycode kKeyLeftControl = 341;
constexpr Keycode kKeyLeftAlt = 342;
constexpr Keycode kKeyLeftSuper = 343;
constexpr Keycode kKeyRightShift = 344;
constexpr Keycode kKeyRightControl = 345;
constexpr Keycode kKeyRightAlt = 346;
constexpr Keycode kKeyRightSuper = 347;
constexpr Keycode kKeyMenu = 348;
constexpr Keycode kKeyLast = kKeyMenu;

extern auto c_str(Keycode code) -> const char*;

using KeyModifierMask = uint32_t;
constexpr KeyModifierMask kKeyModifierBitShift = 0x0001U;
constexpr KeyModifierMask kKeyModifierBitCtrl = 0x0002U;
constexpr KeyModifierMask kKeyModifierBitAlt = 0x0004U;
constexpr KeyModifierMask kKeyModifierBitSuper = 0x0008U;

using MouseButton = uint32_t;
constexpr MouseButton kMouseButtonLeft = 0;
constexpr MouseButton kMouseButtonRight = 1;
constexpr MouseButton kMouseButtonMiddle = 2;
constexpr MouseButton kMouseButtonWheel = 3;
constexpr MouseButton kMouseButtonX1 = 4;
constexpr MouseButton kMouseButtonX2 = 5;
constexpr MouseButton kMouseButtonCount = 6;
constexpr MouseButton kMouseButtonNone = 0xffffffff;

extern auto c_str(MouseButton button) -> const char*;

enum class InputEventType : std::uint32_t {
  kNoEvent = 0,
  kKeyEvent = 1,
  kTextEvent = 2,
  kCharEvent = 3,
  kWindowFocusEvent = 4,
  kCursorEnterEvent = 5,
  kMouseMoveEvent = 6,
  kMouseButtonEvent = 7,
  kMouseWheelEvent = 8,
  kWindowResizeEvent = 9,
  kWindowRefreshEvent = 10,
  kWindowCloseEvent = 11,
};

class KeyEvent {
 public:
  signed int keycode;
  uint32_t modifierMask;
  bool pressed;

  [[nodiscard]] auto describe() const -> std::string;
};

class TextEvent {
 public:
  char utf8Text[32];

  [[nodiscard]] auto describe() const -> std::string;
};

class CharEvent {
 public:
  std::uint32_t codepoint;

  [[nodiscard]] auto describe() const -> std::string;
};

class CursorEnterEvent {
 public:
  int entered;

  [[nodiscard]] auto describe() const -> std::string;
};

class MouseMoveEvent {
 public:
  float x;
  float y;
  float dx;
  float dy;
  uint32_t modifierMask;

  [[nodiscard]] auto describe() const -> std::string;
};

class MouseButtonEvent {
 public:
  uint32_t button;
  bool pressed;
  uint32_t modifierMask;

  [[nodiscard]] auto describe() const -> std::string;
};

class MouseWheelEvent {
 public:
  float x;
  float y;
  uint32_t modifierMask;

  [[nodiscard]] auto describe() const -> std::string;
};

class WindowResizeEvent {
 public:
  int width;
  int height;

  [[nodiscard]] auto describe() const -> std::string;
};

class WindowCloseEvent {
 public:
  [[nodiscard]] auto describe() const -> std::string;
};

class WindowRefreshEvent {
 public:
  [[nodiscard]] auto describe() const -> std::string;
};

class WindowFocusEvent {
 public:
  bool focused;

  [[nodiscard]] auto describe() const -> std::string;
};

class InputEvent {
 public:
  InputEventType type;
  int64_t timestampNs;
  bool handled{false};
  union ImguiEventUnion {
    KeyEvent keyEvent;
    TextEvent textEvent;
    CharEvent charEvent;
    WindowFocusEvent windowFocusEvent;
    CursorEnterEvent cursorEnterEvent;
    MouseMoveEvent mouseMoveEvent;
    MouseButtonEvent mouseButtonEvent;
    MouseWheelEvent mouseWheelEvent;
    WindowResizeEvent windowResizeEvent;
    WindowCloseEvent windowCloseEvent;
    WindowRefreshEvent windowRefreshEvent;
    bool dummy;
  } u;

  [[nodiscard]] auto describe() const -> std::string;
};

class InputEventHandler {
 public:
  virtual ~InputEventHandler() noexcept;

  virtual auto dispatchInputEvent(InputEvent& inputEvent) -> bool;
  virtual auto onKeyEvent(const InputEvent&) -> bool { return false; }
  virtual auto onTextEvent(const InputEvent&) -> bool { return false; }
  virtual auto onCharEvent(const InputEvent&) -> bool { return false; }
  virtual auto onWindowFocusEvent(const InputEvent&) -> bool { return false; }
  virtual auto onCursorEnterEvent(const InputEvent&) -> bool { return false; }
  virtual auto onMouseMoveEvent(const InputEvent&) -> bool { return false; }
  virtual auto onMouseButtonEvent(const InputEvent&) -> bool { return false; }
  virtual auto onMouseWheelEvent(const InputEvent&) -> bool { return false; }
  virtual auto onWindowResizeEvent(const InputEvent&) -> bool { return false; }
  virtual auto onWindowCloseEvent(const InputEvent&) -> bool { return false; }
  virtual auto onWindowRefreshEvent(const InputEvent&) -> bool { return false; }
};

}  // namespace vkit::window
