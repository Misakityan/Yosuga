#ifdef __APPLE__
#include <CoreGraphics/CoreGraphics.h>
#endif
#ifdef __linux__
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/XKBlib.h>
#include <X11/extensions/XTest.h>
#endif
#include <iostream>
#include <random>
#include <map>
#include <cstring>

#include "./Keyboard.h"
#include "./Utils.h"

namespace Robot {

int Keyboard::delay = 1;

const char Keyboard::INVALID_ASCII = static_cast<char>(0xFF);

std::thread Keyboard::keyPressThread;
std::atomic<bool> Keyboard::continueHolding(false);
std::set<char> Keyboard::heldAsciiChars;
std::set<Keyboard::SpecialKey> Keyboard::heldSpecialKeys;

#ifdef __linux__
  Display* Keyboard::display = nullptr;
  Window Keyboard::rootWindow = 0;

  void Keyboard::InitializeX11() {
    if (display == nullptr) {
      display = XOpenDisplay(nullptr);
      if (display == nullptr) {
        throw std::runtime_error("Cannot open X11 display");
      }
      rootWindow = DefaultRootWindow(display);
    }
  }

  void Keyboard::CleanupX11() {
    if (display != nullptr) {
      XCloseDisplay(display);
      display = nullptr;
    }
  }
#endif
#ifdef __linux__
  static bool NeedShiftForKeySym(KeySym keysym) {
    // 检查这个键是否需要 Shift 键配合
    switch (keysym) {
      case XK_exclam:      // !
      case XK_at:          // @
      case XK_numbersign:  // #
      case XK_dollar:      // $
      case XK_percent:     // %
      case XK_asciicircum: // ^
      case XK_ampersand:   // &
      case XK_asterisk:    // *
      case XK_parenleft:   // (
      case XK_parenright:  // )
      case XK_underscore:  // _
      case XK_plus:        // +
      case XK_braceleft:   // {
      case XK_braceright:  // }
      case XK_bar:         // |
      case XK_colon:       // :
      case XK_quotedbl:    // "
      case XK_less:        // <
      case XK_greater:     // >
      case XK_question:    // ?
      case XK_asciitilde:  // ~
        return true;

        // 大写字母需要 Shift 键
      case XK_A:
      case XK_B:
      case XK_C:
      case XK_D:
      case XK_E:
      case XK_F:
      case XK_G:
      case XK_H:
      case XK_I:
      case XK_J:
      case XK_K:
      case XK_L:
      case XK_M:
      case XK_N:
      case XK_O:
      case XK_P:
      case XK_Q:
      case XK_R:
      case XK_S:
      case XK_T:
      case XK_U:
      case XK_V:
      case XK_W:
      case XK_X:
      case XK_Y:
      case XK_Z:
        return true;

      default:
        return false;
    }
  }

  static bool NeedShiftForChar(const char c) {
    // 简单判断：大写字母和特殊符号需要 Shift
    if (std::isupper(c)) {
      return true;
    }

    // 特殊符号需要 Shift
    const char* shiftChars = "!@#$%^&*()_+{}|:\"<>?~";
    return (strchr(shiftChars, c) != nullptr);
  }
#endif


void Keyboard::HoldStart(char asciiChar) {
  if (heldAsciiChars.empty() && heldSpecialKeys.empty()) {
    continueHolding = true;
    keyPressThread = std::thread(KeyHoldThread);
  }
  heldAsciiChars.insert(asciiChar);
}

void Keyboard::HoldStart(SpecialKey specialKey) {
  if (heldAsciiChars.empty() && heldSpecialKeys.empty()) {
    continueHolding = true;
    keyPressThread = std::thread(KeyHoldThread);
  }
  heldSpecialKeys.insert(specialKey);
}

void Keyboard::HoldStop(char asciiChar) {
  heldAsciiChars.erase(asciiChar);
  if (heldAsciiChars.empty() && heldSpecialKeys.empty()) {
    continueHolding = false;
    if (keyPressThread.joinable()) {
      keyPressThread.join();
    }
  }
  Release(asciiChar);
}

void Keyboard::HoldStop(SpecialKey specialKey) {
  heldSpecialKeys.erase(specialKey);
  if (heldAsciiChars.empty() && heldSpecialKeys.empty()) {
    continueHolding = false;
    if (keyPressThread.joinable()) {
      keyPressThread.join();
    }
  }
  Release(specialKey);
}

void Keyboard::KeyHoldThread() {
  while (continueHolding) {
    for (char asciiChar : heldAsciiChars) {
      Press(asciiChar);
    }
    for (SpecialKey specialKey : heldSpecialKeys) {
      Press(specialKey);
    }
    Robot::delay(50);
  }

  for (char asciiChar : heldAsciiChars) {
    Release(asciiChar);
  }
  for (SpecialKey specialKey : heldSpecialKeys) {
    Release(specialKey);
  }
}

void Keyboard::Type(const std::string &query) {
  for (const char c : query) {
    if (!KeyUtils::IsValidAscii(c)) {
      std::cerr << "Warning: Skipping invalid ASCII character: " << static_cast<int>(c) << std::endl;
      continue;
    }
    Click(c);
  }
}

void Keyboard::TypeHumanLike(const std::string &query) {
  std::normal_distribution<double> distribution(75, 25);
  std::random_device rd;
  std::mt19937 engine(rd());

  for (char c : query) {
    if (!KeyUtils::IsValidAscii(c)) {
      std::cerr << "Warning: Skipping invalid ASCII character: " << static_cast<int>(c) << std::endl;
      continue;
    }

    Click(c);
    Robot::delay((int)distribution(engine));
  }
}

void Keyboard::Click(char asciiChar) {
#ifdef __linux__
    InitializeX11();
    // Linux 特殊处理：对于需要 Shift 的字符，先按下 Shift
    bool needShift = NeedShiftForChar(asciiChar);

    if (needShift) {
      // 获取 Shift 键的键码
      KeyCode shiftKeyCode = XKeysymToKeycode(display, XK_Shift_L);
      if (shiftKeyCode != 0) {
        XTestFakeKeyEvent(display, shiftKeyCode, True, CurrentTime);
        XFlush(display);
        Robot::delay(delay);
      }
    }
  Press(asciiChar);
  Release(asciiChar);
    if (needShift) {
      // 释放 Shift 键
      KeyCode shiftKeyCode = XKeysymToKeycode(display, XK_Shift_L);
      if (shiftKeyCode != 0) {
        XTestFakeKeyEvent(display, shiftKeyCode, False, CurrentTime);
        XFlush(display);
        Robot::delay(delay);
      }
    }
#endif

#ifdef __APPLE__
  // macOS实现
  bool needShift = KeyUtils::NeedsShift(asciiChar);~
  char baseKey = KeyUtils::GetBaseKey(asciiChar);
  auto it = asciiToVirtualKeyMap.find(baseKey);
  if (it == asciiToVirtualKeyMap.end()) {
    std::cerr << "Warning: Character not found in key map: " << asciiChar << std::endl;
    return;
  }

  CGKeyCode keycode = static_cast<CGKeyCode>(it->second);
  CGEventSourceRef source = CGEventSourceCreate(kCGEventSourceStateHIDSystemState);

  if (needShift) {
    // 按下Shift键
    CGEventRef shiftDown = CGEventCreateKeyboardEvent(source, kVK_Shift, true);
    CGEventPost(kCGHIDEventTap, shiftDown);
    CFRelease(shiftDown);
    Robot::delay(delay);
  }

  // 按下并释放目标键
  CGEventRef keyDown = CGEventCreateKeyboardEvent(source, keycode, true);
  CGEventRef keyUp = CGEventCreateKeyboardEvent(source, keycode, false);

  CGEventPost(kCGHIDEventTap, keyDown);
  Robot::delay(delay);
  CGEventPost(kCGHIDEventTap, keyUp);
  Robot::delay(delay);

  if (needShift) {
    // 释放Shift键
    CGEventRef shiftUp = CGEventCreateKeyboardEvent(source, kVK_Shift, false);
    CGEventPost(kCGHIDEventTap, shiftUp);
    CFRelease(shiftUp);
    Robot::delay(delay);
  }

  CFRelease(keyDown);
  CFRelease(keyUp);
  CFRelease(source);

#endif

#ifdef _WIN32
  // Windows实现
  bool needShift = KeyUtils::NeedsShift(asciiChar);
  char baseKey = KeyUtils::GetBaseKey(asciiChar);

  // 获取基础键的虚拟键码
  SHORT vkAndShift = VkKeyScan(baseKey);
  if (vkAndShift == -1) {
    std::cerr << "Warning: Cannot get virtual key for character: " << asciiChar << std::endl;
    return;
  }

  WORD keycode = static_cast<WORD>(vkAndShift & 0xFF);

  std::vector<INPUT> inputs;

  if (needShift) {
    // Shift键按下
    INPUT shiftDown = {0};
    shiftDown.type = INPUT_KEYBOARD;
    shiftDown.ki.wVk = VK_SHIFT;
    inputs.push_back(shiftDown);
  }

  // 目标键按下
  INPUT keyDown = {0};
  keyDown.type = INPUT_KEYBOARD;
  keyDown.ki.wVk = keycode;
  inputs.push_back(keyDown);

  // 目标键释放
  INPUT keyUp = {0};
  keyUp.type = INPUT_KEYBOARD;
  keyUp.ki.wVk = keycode;
  keyUp.ki.dwFlags = KEYEVENTF_KEYUP;
  inputs.push_back(keyUp);

  if (needShift) {
    // Shift键释放
    INPUT shiftUp = {0};
    shiftUp.type = INPUT_KEYBOARD;
    shiftUp.ki.wVk = VK_SHIFT;
    shiftUp.ki.dwFlags = KEYEVENTF_KEYUP;
    inputs.push_back(shiftUp);
  }

  // 发送所有输入
  SendInput(static_cast<UINT>(inputs.size()), inputs.data(), sizeof(INPUT));
#endif
}

void Keyboard::Click(SpecialKey specialKey) {
  Press(specialKey);
  Release(specialKey);
}

void Keyboard::Press(char asciiChar) {
  KeyCode keycode = AsciiToVirtualKey(asciiChar);
#ifdef __APPLE__
  CGEventSourceRef source =
      CGEventSourceCreate(kCGEventSourceStateHIDSystemState);
  CGEventRef event = CGEventCreateKeyboardEvent(source, keycode, true);
  CGEventPost(kCGHIDEventTap, event);

  CFRelease(event);
  CFRelease(source);
#endif

#ifdef _WIN32
  INPUT input = {0};
  input.type = INPUT_KEYBOARD;
  input.ki.wVk = keycode;
  SendInput(1, &input, sizeof(INPUT));
#endif

#ifdef __linux__
    InitializeX11();
    KeyCode xkeycode = XKeysymToKeycode(display, keycode);
    if (xkeycode != 0) {
      XTestFakeKeyEvent(display, xkeycode, True, CurrentTime);
      XFlush(display);
    }
#endif
  Robot::delay(delay);
}

void Keyboard::Press(SpecialKey specialKey) {
  KeyCode keycode = SpecialKeyToVirtualKey(specialKey);
#ifdef __APPLE__
  CGEventRef event = CGEventCreateKeyboardEvent(nullptr, keycode, true);
  CGEventPost(kCGHIDEventTap, event);
  CFRelease(event);
#endif

#ifdef _WIN32
  INPUT input = {0};
  input.type = INPUT_KEYBOARD;
  input.ki.wVk = keycode;
  SendInput(1, &input, sizeof(INPUT));
#endif

#ifdef __linux__
    InitializeX11();
    KeyCode xkeycode = XKeysymToKeycode(display, keycode);
    if (xkeycode != 0) {
      XTestFakeKeyEvent(display, xkeycode, True, CurrentTime);
      XFlush(display);
    }
#endif
  Robot::delay(delay);
}

void Keyboard::Release(char asciiChar) {
  KeyCode keycode = AsciiToVirtualKey(asciiChar);
#ifdef __APPLE__
  CGEventRef event =
      CGEventCreateKeyboardEvent(nullptr, (CGKeyCode)keycode, false);
  CGEventPost(kCGHIDEventTap, event);
  CFRelease(event);
#endif

#ifdef _WIN32
  INPUT input = {0};
  input.type = INPUT_KEYBOARD;
  input.ki.wVk = keycode;
  input.ki.dwFlags = KEYEVENTF_KEYUP;
  SendInput(1, &input, sizeof(INPUT));
#endif

#ifdef __linux__
    InitializeX11();
    KeyCode xkeycode = XKeysymToKeycode(display, keycode);
    if (xkeycode != 0) {
      XTestFakeKeyEvent(display, xkeycode, False, CurrentTime);
      XFlush(display);
    }
#endif
  Robot::delay(delay);
}

void Keyboard::Release(SpecialKey specialKey) {
  KeyCode keycode = SpecialKeyToVirtualKey(specialKey);
#ifdef __APPLE__
  CGEventRef event =
      CGEventCreateKeyboardEvent(nullptr, (CGKeyCode)keycode, false);
  CGEventPost(kCGHIDEventTap, event);
  CFRelease(event);
#endif

#ifdef _WIN32
  INPUT input = {0};
  input.type = INPUT_KEYBOARD;
  input.ki.wVk = keycode;
  input.ki.dwFlags = KEYEVENTF_KEYUP;
  SendInput(1, &input, sizeof(INPUT));
#endif

#ifdef __linux__
    InitializeX11();
    KeyCode xkeycode = XKeysymToKeycode(display, keycode);
    if (xkeycode != 0) {
      XTestFakeKeyEvent(display, xkeycode, False, CurrentTime);
      XFlush(display);
    }
#endif
  Robot::delay(delay);
}

KeyCode Keyboard::SpecialKeyToVirtualKey(SpecialKey specialKey) {
  return specialKeyToVirtualKeyMap.at(specialKey);
}

KeyCode Keyboard::AsciiToVirtualKey(char asciiChar) {
#ifdef __APPLE__
  auto it = asciiToVirtualKeyMap.find(asciiChar);
  if (it == asciiToVirtualKeyMap.end()) {
    std::cerr
        << "Warning: Character not found in the virtual key map. Ignoring..."
        << std::endl;
    return 0xFFFF;  // Return an invalid keycode
  }
  return static_cast<KeyCode>(it->second);
#endif

#ifdef _WIN32
  // Windows: VkKeyScan 函数返回一个short，高字节是shift状态，低字节是虚拟键码
  SHORT vkAndShift = VkKeyScan(asciiChar);
  if (vkAndShift == -1) {
    std::cerr
        << "Warning: Character not found in the virtual key map. Ignoring..."
        << std::endl;
    return 0xFFFF;  // Return an invalid keycode
  }
  // 返回虚拟键码（低字节）
  return static_cast<KeyCode>(vkAndShift & 0xFF);
#endif

#ifdef __linux__
  // Convert ASCII to X11 KeySym
  // For Linux, we need to handle case sensitivity properly
  if (std::isalpha(asciiChar)) {
    // For letters, use lowercase KeySym
    switch (char lowerChar = std::tolower(asciiChar)) {
      case 'a': return XK_a;
      case 'b': return XK_b;
      case 'c': return XK_c;
      case 'd': return XK_d;
      case 'e': return XK_e;
      case 'f': return XK_f;
      case 'g': return XK_g;
      case 'h': return XK_h;
      case 'i': return XK_i;
      case 'j': return XK_j;
      case 'k': return XK_k;
      case 'l': return XK_l;
      case 'm': return XK_m;
      case 'n': return XK_n;
      case 'o': return XK_o;
      case 'p': return XK_p;
      case 'q': return XK_q;
      case 'r': return XK_r;
      case 's': return XK_s;
      case 't': return XK_t;
      case 'u': return XK_u;
      case 'v': return XK_v;
      case 'w': return XK_w;
      case 'x': return XK_x;
      case 'y': return XK_y;
      case 'z': return XK_z;
    }
  }

  // For numbers and special characters
  switch (asciiChar) {
    // Numbers
    case '0': return XK_0;
    case '1': return XK_1;
    case '2': return XK_2;
    case '3': return XK_3;
    case '4': return XK_4;
    case '5': return XK_5;
    case '6': return XK_6;
    case '7': return XK_7;
    case '8': return XK_8;
    case '9': return XK_9;

    // Special characters that need shift
    case '!': return XK_exclam;      // Shift+1
    case '@': return XK_at;          // Shift+2
    case '#': return XK_numbersign;  // Shift+3
    case '$': return XK_dollar;      // Shift+4
    case '%': return XK_percent;     // Shift+5
    case '^': return XK_asciicircum; // Shift+6
    case '&': return XK_ampersand;   // Shift+7
    case '*': return XK_asterisk;    // Shift+8
    case '(': return XK_parenleft;   // Shift+9
    case ')': return XK_parenright;  // Shift+0
    case '_': return XK_underscore;  // Shift+-
    case '+': return XK_plus;        // Shift+=
    case '{': return XK_braceleft;   // Shift+[
    case '}': return XK_braceright;  // Shift+]
    case '|': return XK_bar;         // Shift+\
    case ':': return XK_colon;       // Shift+;
    case '"': return XK_quotedbl;    // Shift+'
    case '<': return XK_less;        // Shift+,
    case '>': return XK_greater;     // Shift+.
    case '?': return XK_question;    // Shift+/
    case '~': return XK_asciitilde;  // Shift+`

    // Special characters without shift
    case ' ': return XK_space;
    case '\t': return XK_Tab;
    case '\n': return XK_Return;
    case '\b': return XK_BackSpace;
    case 27: return XK_Escape;  // ESC

    case '-': return XK_minus;
    case '=': return XK_equal;
    case '[': return XK_bracketleft;
    case ']': return XK_bracketright;
    case '\\': return XK_backslash;
    case ';': return XK_semicolon;
    case '\'': return XK_apostrophe;
    case ',': return XK_comma;
    case '.': return XK_period;
    case '/': return XK_slash;
    case '`': return XK_grave;

    case ':': return XK_colon;

    default:
      std::cerr << "Warning: Character " << static_cast<int>(asciiChar)
                << " not mapped to X11 KeySym. Using XK_space instead."
                << std::endl;
      return XK_space;
  }
#endif
}

#ifdef __APPLE__
std::map<Keyboard::SpecialKey, KeyCode> Keyboard::specialKeyToVirtualKeyMap = {
    {Keyboard::BACKSPACE, kVK_Delete},
    {Keyboard::ENTER, kVK_Return},
    {Keyboard::TAB, kVK_Tab},
    {Keyboard::ESCAPE, kVK_Escape},
    {Keyboard::UP, kVK_UpArrow},
    {Keyboard::DOWN, kVK_DownArrow},
    {Keyboard::RIGHT, kVK_RightArrow},
    {Keyboard::LEFT, kVK_LeftArrow},
    {Keyboard::META, kVK_Command},
    {Keyboard::ALT, kVK_Option},
    {Keyboard::CONTROL, kVK_Control},
    {Keyboard::SHIFT, kVK_Shift},
    {Keyboard::CAPSLOCK, kVK_CapsLock},
    {Keyboard::F1, kVK_F1},
    {Keyboard::F2, kVK_F2},
    {Keyboard::F3, kVK_F3},
    {Keyboard::F4, kVK_F4},
    {Keyboard::F5, kVK_F5},
    {Keyboard::F6, kVK_F6},
    {Keyboard::F7, kVK_F7},
    {Keyboard::F8, kVK_F8},
    {Keyboard::F9, kVK_F9},
    {Keyboard::F10, kVK_F10},
    {Keyboard::F11, kVK_F11},
    {Keyboard::F12, kVK_F12}};
#endif



#ifdef __linux__
  std::map<Keyboard::SpecialKey, KeyCode> Keyboard::specialKeyToVirtualKeyMap = {
    {Keyboard::BACKSPACE, XK_BackSpace},
    {Keyboard::ENTER, XK_Return},
    {Keyboard::TAB, XK_Tab},
    {Keyboard::ESCAPE, XK_Escape},
    {Keyboard::UP, XK_Up},
    {Keyboard::DOWN, XK_Down},
    {Keyboard::RIGHT, XK_Right},
    {Keyboard::LEFT, XK_Left},
    {Keyboard::META, XK_Super_L},  // Super/Windows key
    {Keyboard::ALT, XK_Alt_L},
    {Keyboard::CONTROL, XK_Control_L},
    {Keyboard::SHIFT, XK_Shift_L},
    {Keyboard::CAPSLOCK, XK_Caps_Lock},
    {Keyboard::F1, XK_F1},
    {Keyboard::F2, XK_F2},
    {Keyboard::F3, XK_F3},
    {Keyboard::F4, XK_F4},
    {Keyboard::F5, XK_F5},
    {Keyboard::F6, XK_F6},
    {Keyboard::F7, XK_F7},
    {Keyboard::F8, XK_F8},
    {Keyboard::F9, XK_F9},
    {Keyboard::F10, XK_F10},
    {Keyboard::F11, XK_F11},
    {Keyboard::F12, XK_F12}};
#endif

  char Keyboard::VirtualKeyToAscii(KeyCode virtualKey) {
#ifdef __APPLE__
    auto map = asciiToVirtualKeyMap;
  auto it = std::find_if(
      map.begin(),
      map.end(),
      [virtualKey](const std::pair<char, int> &p) {
        return p.second == virtualKey;
      }
  );

  if (it == map.end()) {
    return INVALID_ASCII;
  }

  return it->first;
#endif

#ifdef _WIN32
    // Convert the virtual key code to a scan code
    UINT scanCode = MapVirtualKey(virtualKey, MAPVK_VK_TO_VSC);

    // Convert the scan code to the corresponding character
    char character = 0;
    BYTE keyboardState[256] = {0};
    GetKeyboardState(keyboardState);
    wchar_t buffer[2];
    if (ToUnicode(virtualKey, scanCode, keyboardState, buffer, 2, 0) == 1) {
      character = static_cast<char>(buffer[0]);
    }
    return character;
#endif

#ifdef __linux__
    InitializeX11();

    // Get the keycode from keysym
    KeyCode keycode = XKeysymToKeycode(display, virtualKey);
    if (keycode == 0) {
      return INVALID_ASCII;
    }

    // Get the current keyboard state
    char keys[32];
    XQueryKeymap(display, keys);

    // Get the current modifier state
    XKeyEvent event;
    event.display = display;
    event.window = rootWindow;
    event.root = rootWindow;
    event.subwindow = None;
    event.time = CurrentTime;
    event.x = event.y = event.x_root = event.y_root = 0;
    event.same_screen = True;
    event.keycode = keycode;
    event.state = 0;

    // Try to get the character with different modifier states
    KeySym keysym;
    XLookupString(&event, nullptr, 0, &keysym, nullptr);

    // Convert keysym to ASCII if possible
    if (keysym >= 0x20 && keysym <= 0x7E) {  // Printable ASCII range
      return static_cast<char>(keysym);
    }

    return INVALID_ASCII;
#endif

  }

  Keyboard::SpecialKey Keyboard::VirtualKeyToSpecialKey(KeyCode virtualKey) {
#ifdef __APPLE__
    switch (virtualKey) {
    case 123:
      return Keyboard::LEFT;
    case 124:
      return Keyboard::RIGHT;
    case 125:
      return Keyboard::DOWN;
    case 126:
      return Keyboard::UP;
    case 36:
      return Keyboard::ENTER;
    case 48:
      return Keyboard::TAB;
    case 51:
      return Keyboard::BACKSPACE;
    case 53:
      return Keyboard::ESCAPE;
    case 55:
      return Keyboard::META;
    case 56:
      return Keyboard::SHIFT;
    case 57:
      return Keyboard::CAPSLOCK;
    case 58:
      return Keyboard::ALT;
    case 59:
      return Keyboard::CONTROL;
  }
#endif

#ifdef _WIN32
    switch (virtualKey) {
      case VK_LEFT:
        return Keyboard::LEFT;
      case VK_RIGHT:
        return Keyboard::RIGHT;
      case VK_DOWN:
        return Keyboard::DOWN;
      case VK_UP:
        return Keyboard::UP;
      case VK_RETURN:
        return Keyboard::ENTER;
      case VK_TAB:
        return Keyboard::TAB;
      case VK_BACK:
        return Keyboard::BACKSPACE;
      case VK_ESCAPE:
        return Keyboard::ESCAPE;
      case VK_LWIN:
      case VK_RWIN:
        return Keyboard::META;
      case VK_SHIFT:
        return Keyboard::SHIFT;
      case VK_CAPITAL:
        return Keyboard::CAPSLOCK;
      case VK_MENU:
        return Keyboard::ALT;
      case VK_CONTROL:
        return Keyboard::CONTROL;
    }
#endif

#ifdef __linux__
    switch (virtualKey) {
      case XK_Left:
        return Keyboard::LEFT;
      case XK_Right:
        return Keyboard::RIGHT;
      case XK_Down:
        return Keyboard::DOWN;
      case XK_Up:
        return Keyboard::UP;
      case XK_Return:
        return Keyboard::ENTER;
      case XK_Tab:
        return Keyboard::TAB;
      case XK_BackSpace:
        return Keyboard::BACKSPACE;
      case XK_Escape:
        return Keyboard::ESCAPE;
      case XK_Super_L:
      case XK_Super_R:
        return Keyboard::META;
      case XK_Shift_L:
      case XK_Shift_R:
        return Keyboard::SHIFT;
      case XK_Caps_Lock:
        return Keyboard::CAPSLOCK;
      case XK_Alt_L:
      case XK_Alt_R:
        return Keyboard::ALT;
      case XK_Control_L:
      case XK_Control_R:
        return Keyboard::CONTROL;
    }
#endif
    // Default case for all platforms
    return static_cast<Keyboard::SpecialKey>(0);
  }

#ifdef _WIN32
std::map<Keyboard::SpecialKey, KeyCode> Keyboard::specialKeyToVirtualKeyMap = {
    {Keyboard::BACKSPACE, VK_BACK},
    {Keyboard::ENTER, VK_RETURN},
    {Keyboard::TAB, VK_TAB},
    {Keyboard::ESCAPE, VK_ESCAPE},
    {Keyboard::UP, VK_UP},
    {Keyboard::DOWN, VK_DOWN},
    {Keyboard::RIGHT, VK_RIGHT},
    {Keyboard::LEFT, VK_LEFT},
    {Keyboard::META, VK_LWIN},
    {Keyboard::ALT, VK_MENU},
    {Keyboard::CONTROL, VK_CONTROL},
    {Keyboard::SHIFT, VK_SHIFT},
    {Keyboard::CAPSLOCK, VK_CAPITAL},
    {Keyboard::F1, VK_F1},
    {Keyboard::F2, VK_F2},
    {Keyboard::F3, VK_F3},
    {Keyboard::F4, VK_F4},
    {Keyboard::F5, VK_F5},
    {Keyboard::F6, VK_F6},
    {Keyboard::F7, VK_F7},
    {Keyboard::F8, VK_F8},
    {Keyboard::F9, VK_F9},
    {Keyboard::F10, VK_F10},
    {Keyboard::F11, VK_F11},
    {Keyboard::F12, VK_F12}};
#endif

#ifdef __APPLE__
std::map<char, int> Keyboard::asciiToVirtualKeyMap = {
    // 数字
    {'0', kVK_ANSI_0}, {'1', kVK_ANSI_1}, {'2', kVK_ANSI_2},
    {'3', kVK_ANSI_3}, {'4', kVK_ANSI_4}, {'5', kVK_ANSI_5},
    {'6', kVK_ANSI_6}, {'7', kVK_ANSI_7}, {'8', kVK_ANSI_8},
    {'9', kVK_ANSI_9},

    // 字母（只使用小写）
    {'a', kVK_ANSI_A}, {'b', kVK_ANSI_B}, {'c', kVK_ANSI_C},
    {'d', kVK_ANSI_D}, {'e', kVK_ANSI_E}, {'f', kVK_ANSI_F},
    {'g', kVK_ANSI_G}, {'h', kVK_ANSI_H}, {'i', kVK_ANSI_I},
    {'j', kVK_ANSI_J}, {'k', kVK_ANSI_K}, {'l', kVK_ANSI_L},
    {'m', kVK_ANSI_M}, {'n', kVK_ANSI_N}, {'o', kVK_ANSI_O},
    {'p', kVK_ANSI_P}, {'q', kVK_ANSI_Q}, {'r', kVK_ANSI_R},
    {'s', kVK_ANSI_S}, {'t', kVK_ANSI_T}, {'u', kVK_ANSI_U},
    {'v', kVK_ANSI_V}, {'w', kVK_ANSI_W}, {'x', kVK_ANSI_X},
    {'y', kVK_ANSI_Y}, {'z', kVK_ANSI_Z},

    // 特殊字符（不需要Shift的）
    {' ', kVK_Space},
    {'-', kVK_ANSI_Minus},
    {'=', kVK_ANSI_Equal},
    {'[', kVK_ANSI_LeftBracket},
    {']', kVK_ANSI_RightBracket},
    {'\\', kVK_ANSI_Backslash},
    {';', kVK_ANSI_Semicolon},
    {'\'', kVK_ANSI_Quote},
    {',', kVK_ANSI_Comma},
    {'.', kVK_ANSI_Period},
    {'/', kVK_ANSI_Slash},
    {'`', kVK_ANSI_Grave},

    // 需要Shift的字符（映射到对应数字或符号键）
    {'!', kVK_ANSI_1},   // Shift+1
    {'@', kVK_ANSI_2},   // Shift+2
    {'#', kVK_ANSI_3},   // Shift+3
    {'$', kVK_ANSI_4},   // Shift+4
    {'%', kVK_ANSI_5},   // Shift+5
    {'^', kVK_ANSI_6},   // Shift+6
    {'&', kVK_ANSI_7},   // Shift+7
    {'*', kVK_ANSI_8},   // Shift+8
    {'(', kVK_ANSI_9},   // Shift+9
    {')', kVK_ANSI_0},   // Shift+0
    {'_', kVK_ANSI_Minus},  // Shift+-
    {'+', kVK_ANSI_Equal},  // Shift+=
    {'{', kVK_ANSI_LeftBracket},  // Shift+[
    {'}', kVK_ANSI_RightBracket}, // Shift+]
    {'|', kVK_ANSI_Backslash},    // Shift+\
    {':', kVK_ANSI_Semicolon},    // Shift+;
    {'"', kVK_ANSI_Quote},        // Shift+'
    {'<', kVK_ANSI_Comma},        // Shift+,
    {'>', kVK_ANSI_Period},       // Shift+.
    {'?', kVK_ANSI_Slash},        // Shift+/
    {'~', kVK_ANSI_Grave}         // Shift+`
};
#endif
}  // namespace Robot
