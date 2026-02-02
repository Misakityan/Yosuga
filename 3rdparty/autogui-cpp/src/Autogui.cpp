//
// Created by misaki on 2026/1/25.
//

#include "Autogui.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <random>
#include <thread>
#include <cstring>
#ifndef M_PI
# define M_PI		3.14159265358979323846
#endif
namespace AutoGUI {

// 内部辅助函数
namespace {

// 等待指定毫秒数
void delayMs(const int ms) {
  std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

// 将秒转换为毫秒
int secondsToMs(double seconds) { return static_cast<int>(seconds * 1000); }

// 获取当前鼠标位置（内部使用）
Robot::Point getCurrentPosition() { return Robot::Mouse::GetPosition(); }

} // namespace

#if defined(__linux__)
#include <X11/Xlib.h>
#include <X11/extensions/Xrandr.h>
#include <algorithm>

// 获取所有屏幕信息
std::vector<ScreenInfo> getAllScreens() {
    std::vector<ScreenInfo> screens;
    Display* display = XOpenDisplay(nullptr);
    if (!display) return screens;

    XRRScreenResources* resources = XRRGetScreenResources(display, DefaultRootWindow(display));
    if (resources) {
        for (int i = 0; i < resources->noutput; i++) {
            XRROutputInfo* output = XRRGetOutputInfo(display, resources, resources->outputs[i]);
            if (output && output->connection == RR_Connected && output->crtc) {
                XRRCrtcInfo* crtc = XRRGetCrtcInfo(display, resources, output->crtc);
                if (crtc) {
                    ScreenInfo info;
                    info.id = i;
                    info.x = crtc->x;
                    info.y = crtc->y;
                    info.width = crtc->width;
                    info.height = crtc->height;
                    info.isPrimary = (output->name && strcmp(output->name, "primary") == 0);
                    // 或者使用 XRRGetOutputPrimary 来判断
                    screens.push_back(info);
                    XRRFreeCrtcInfo(crtc);
                }
                XRRFreeOutputInfo(output);
            }
        }
        XRRFreeScreenResources(resources);
    }
    XCloseDisplay(display);
    return screens;
}

// 获取当前鼠标所在的屏幕
ScreenInfo getCurrentScreen() {
    auto screens = getAllScreens();
    if (screens.empty()) return {0, 0, 0, 1920, 1080, true};

    // 获取当前鼠标位置（虚拟桌面绝对坐标）
    Robot::Point mouse = position();

    // 找到包含该点的屏幕
    for (const auto& screen : screens) {
        if (mouse.x >= screen.x && mouse.x < screen.x + screen.width &&
            mouse.y >= screen.y && mouse.y < screen.y + screen.height) {
            return screen;
        }
    }

    // 默认返回主屏或第一个屏幕
    auto it = std::find_if(screens.begin(), screens.end(),
                          [](const ScreenInfo& s) { return s.isPrimary; });
    return (it != screens.end()) ? *it : screens[0];
}

// 相对于当前屏幕移动
void moveToOnCurrentScreen(int x, int y, double duration) {
    const ScreenInfo current = getCurrentScreen();
    // 转换为虚拟桌面绝对坐标
    int absX = current.x + x;
    int absY = current.y + y;
    moveTo(absX, absY, duration);
}

// 获取当前屏幕尺寸
Robot::Point getScreenSize(int screenId) {
    if (screenId == -1) {
        ScreenInfo current = getCurrentScreen();
        return {current.width, current.height};
    }
    auto screens = getAllScreens();
    for (const auto& s : screens) {
        if (s.id == screenId) return {s.width, s.height};
    }
    return {1920, 1080};
}

#endif
#if defined(_WIN32)
#include <Windows.h>

  std::vector<ScreenInfo> getAllScreens() {
  std::vector<ScreenInfo> screens;
  EnumDisplayMonitors(nullptr, nullptr,
      [](HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData) -> BOOL {
          auto* screens = reinterpret_cast<std::vector<ScreenInfo>*>(dwData);
          MONITORINFOEX info;
          info.cbSize = sizeof(info);
          if (GetMonitorInfo(hMonitor, &info)) {
              ScreenInfo si;
              si.id = screens->size();
              si.x = info.rcMonitor.left;
              si.y = info.rcMonitor.top;
              si.width = info.rcMonitor.right - info.rcMonitor.left;
              si.height = info.rcMonitor.bottom - info.rcMonitor.top;
              si.isPrimary = (info.dwFlags & MONITORINFOF_PRIMARY) != 0;
              screens->push_back(si);
          }
          return TRUE;
      }, reinterpret_cast<LPARAM>(&screens));
  return screens;
}

ScreenInfo getCurrentScreen() {
  // 获取当前鼠标位置
  POINT pt;
  GetCursorPos(&pt);
  // 查找包含该点的显示器
  HMONITOR hMonitor = MonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST);
  MONITORINFOEX info;
  info.cbSize = sizeof(info);
  GetMonitorInfo(hMonitor, &info);

  ScreenInfo si;
  si.x = info.rcMonitor.left;
  si.y = info.rcMonitor.top;
  si.width = info.rcMonitor.right - info.rcMonitor.left;
  si.height = info.rcMonitor.bottom - info.rcMonitor.top;
  return si;
}

void moveToOnCurrentScreen(int x, int y, double duration) {
  ScreenInfo current = getCurrentScreen();
  moveTo(current.x + x, current.y + y, duration);
}
#endif


// 实现主要API函数
void moveTo(int x, int y, double duration) {
  Robot::Point target{x, y};

  if (duration > 0.0) {
    Robot::Mouse::MoveSmooth(target);
  } else {
    Robot::Mouse::Move(target);
  }
}

void moveRel(int xOffset, int yOffset, double duration) {
  Robot::Point current = getCurrentPosition();
  Robot::Point target{current.x + xOffset, current.y + yOffset};

  moveTo(target.x, target.y, duration);
}

void click(int x, int y, Button button, int clicks, double interval) {
  // 如果提供了坐标，先移动到该位置
  if (x >= 0 && y >= 0) {
    moveTo(x, y);
    delayMs(10);
  }

  Robot::MouseButton robotButton = toRobotButton(button);

  for (int i = 0; i < clicks; i++) {
    if (clicks == 2 && i == 0) {
      // 双击
      Robot::Mouse::DoubleClick(robotButton);
    } else {
      Robot::Mouse::Click(robotButton);
    }

    // 如果不是最后一次点击，并且设置了间隔，则等待
    if (i < clicks - 1 && interval > 0) {
      sleep(interval);
    }
  }
}

void leftDouble(int x, int y) { click(x, y, Button::LEFT, 2); }

void rightSingle(int x, int y) { click(x, y, Button::RIGHT, 1); }

void middleClick(int x, int y) { click(x, y, Button::MIDDLE, 1); }

void mouseDown(Button button, int x, int y) {
  // 如果提供了坐标，先移动到该位置
  if (x >= 0 && y >= 0) {
    moveTo(x, y);
    delayMs(10);
  }

  Robot::Mouse::ToggleButton(true, toRobotButton(button));
}

void mouseUp(Button button, int x, int y) {
  // 如果提供了坐标，先移动到该位置
  if (x >= 0 && y >= 0) {
    moveTo(x, y);
    delayMs(10);
  }

  Robot::Mouse::ToggleButton(false, toRobotButton(button));
}

void drag(const int x1, const int y1, const int x2, const int y2,
          const double duration, const Button button) {
  // 参数验证
  if (!isValidCoord(x1, y1)) {
    throw AutoGUIException("Invalid start coordinates: (" + std::to_string(x1) +
                           ", " + std::to_string(y1) + ")");
  }
  if (!isValidCoord(x2, y2)) {
    throw AutoGUIException("Invalid end coordinates: (" + std::to_string(x2) +
                           ", " + std::to_string(y2) + ")");
  }
  if (duration < 0) {
    throw AutoGUIException("Duration cannot be negative");
  }
  Robot::MouseButton robotButton = toRobotButton(button);
  // 移动到起始位置
  Robot::Point start{x1, y1};
  Robot::Mouse::Move(start);
  delayMs(10); // 短暂延迟确保移动完成
  // 按下鼠标按钮
  Robot::Mouse::ToggleButton(true, robotButton);
  delayMs(10); // 短暂延迟确保按钮按下
  // 拖动到目标位置
  Robot::Point end{x2, y2};
  if (duration > 0.0) {
    // 使用平滑拖动
    Robot::Mouse::DragSmooth(end);
  } else {
    // 立即拖动
    Robot::Mouse::Drag(end);
  }
  // 释放鼠标按钮
  Robot::Mouse::ToggleButton(false, robotButton);
}

void dragTo(const int x, const int y, const double duration,
            const Button button) {
  // 获取当前位置
  Robot::Point current = getCurrentPosition();
  // 如果当前位置就是目标位置，则只做点击操作
  if (current.x == x && current.y == y) {
    click(x, y, button);
    return;
  }
  // 调用 drag 函数
  drag(current.x, current.y, x, y, duration, button);
}

void dragRel(const int xOffset, const int yOffset, const double duration,
             const Button button) {
  // 获取当前位置
  const Robot::Point current = getCurrentPosition();
  // 计算目标位置
  const int targetX = current.x + xOffset;
  const int targetY = current.y + yOffset;
  // 如果偏移量为0，则只做点击操作
  if (xOffset == 0 && yOffset == 0) {
    click(current.x, current.y, button);
    return;
  }
  // 调用 drag 函数
  drag(current.x, current.y, targetX, targetY, duration, button);
}

Robot::Point position() { return getCurrentPosition(); }

void scroll(int clicks, int x) {
  // 注意：正数向上滚动，负数向下滚动
  // 与AutoGUI一致
  Robot::Mouse::ScrollBy(clicks, x);
}

void type(const std::string &text, double interval) {
  if (interval > 0.0) {
    // 有间隔的输入
    for (char c : text) {
      Robot::Keyboard::Click(c);
      if (interval > 0) {
        sleep(interval);
      }
    }
  } else {
    // 无间隔快速输入
    Robot::Keyboard::Type(text);
  }
}

void press(const std::string &key) {
  std::string lowerKey = toLower(key);

  if (lowerKey.length() == 1) {
    // 单个字符键
    Robot::Keyboard::Click(lowerKey[0]);
  } else if (isSpecialKey(lowerKey)) {
    // 特殊键
    Robot::Keyboard::SpecialKey specialKey = stringToSpecialKey(lowerKey);
    Robot::Keyboard::Click(specialKey);
  } else {
    throw AutoGUIException("Unknown key: " + key);
  }
}

void keyDown(const std::string &key) {
  std::string lowerKey = toLower(key);

  if (lowerKey.length() == 1) {
    // 单个字符键
    Robot::Keyboard::Press(lowerKey[0]);
  } else if (isSpecialKey(lowerKey)) {
    // 特殊键
    Robot::Keyboard::SpecialKey specialKey = stringToSpecialKey(lowerKey);
    Robot::Keyboard::Press(specialKey);
  } else {
    throw AutoGUIException("Unknown key: " + key);
  }
}

void keyUp(const std::string &key) {
  std::string lowerKey = toLower(key);

  if (lowerKey.length() == 1) {
    // 单个字符键
    Robot::Keyboard::Release(lowerKey[0]);
  } else if (isSpecialKey(lowerKey)) {
    // 特殊键
    Robot::Keyboard::SpecialKey specialKey = stringToSpecialKey(lowerKey);
    Robot::Keyboard::Release(specialKey);
  } else {
    throw AutoGUIException("Unknown key: " + key);
  }
}

void hotkey(const std::initializer_list<std::string> &keys) {
  // 按下所有键
  for (const auto &key : keys) {
    keyDown(key);
  }

  delayMs(50);

  // 释放所有键（按相反顺序）
  const std::string *end = keys.end();
  const std::string *begin = keys.begin();
  for (const std::string *it = end - 1; it >= begin; --it) {
    keyUp(*it);
  }
}

void hotkey(const std::vector<std::string> &keys) {
  // 按下所有键
  for (const auto &key : keys) {
    keyDown(key);
  }

  delayMs(50);

  // 释放所有键（按相反顺序）
  for (auto it = keys.rbegin(); it != keys.rend(); ++it) {
    keyUp(*it);
  }
}

void sleep(double seconds) {
  if (seconds > 0) {
    delayMs(secondsToMs(seconds));
  }
}

#if defined(_WIN32)
#include <Windows.h>

#elif defined(__APPLE__)
#include <CoreGraphics/CoreGraphics.h>

#elif defined(__linux__)
#include <X11/Xlib.h>
#include <X11/extensions/Xrandr.h>
#endif

// 平台特定函数实现
Robot::Point size() {
  // 平台特定实现
#ifdef _WIN32
// Windows实现
  int width = GetSystemMetrics(SM_CXSCREEN);
  int height = GetSystemMetrics(SM_CYSCREEN);
  return {width, height};

#elif defined(__APPLE__)
// macOS实现
  CGRect mainDisplayBounds = CGDisplayBounds(CGMainDisplayID());
  int width = static_cast<int>(CGRectGetWidth(mainDisplayBounds));
  int height = static_cast<int>(CGRectGetHeight(mainDisplayBounds));
  return {width, height};

#elif defined(__linux__)
  Display *display = XOpenDisplay(nullptr);
  if (!display) return {1920, 1080};

  int width = 0, height = 0;

  // 使用 XRRGetScreenResources (兼容 XRandR 1.2+，比 Current 版本兼容性更好)
  XRRScreenResources *resources = XRRGetScreenResources(display, DefaultRootWindow(display));
  if (resources) {
    RROutput target_output = None;

    // 条件编译：只在 XRandR 1.3+ 时使用 Primary Output 功能
#if defined(RANDR_MAJOR) && defined(RANDR_MINOR)
#if RANDR_MAJOR > 1 || (RANDR_MAJOR == 1 && RANDR_MINOR >= 3)
    target_output = XRRGetOutputPrimary(display, DefaultRootWindow(display));
#endif
#endif

    // 如果没有获取到主显示器（或版本太低），使用第一个已连接的显示器
    if (target_output == None) {
      for (int i = 0; i < resources->noutput; i++) {
        XRROutputInfo *info = XRRGetOutputInfo(display, resources, resources->outputs[i]);
        if (info) {
          if (info->connection == 0) {
            target_output = resources->outputs[i];
            XRRFreeOutputInfo(info);
            break;
          }
          XRRFreeOutputInfo(info);
        }
      }
    }

    // 获取选中显示器的分辨率
    if (target_output != None) {
      XRROutputInfo *output_info = XRRGetOutputInfo(display, resources, target_output);
      if (output_info && output_info->crtc) {
        XRRCrtcInfo *crtc_info = XRRGetCrtcInfo(display, resources, output_info->crtc);
        if (crtc_info) {
          width = static_cast<int>(crtc_info->width);
          height = static_cast<int>(crtc_info->height);
          XRRFreeCrtcInfo(crtc_info);
        }
        XRRFreeOutputInfo(output_info);
      }
    }
    XRRFreeScreenResources(resources);
  }

  XCloseDisplay(display);
  return (width > 0 && height > 0) ? Robot::Point{width, height} : Robot::Point{1920, 1080};

#else
  // 其他平台
  return {1920, 1080}; // 默认值
#endif
}

// 辅助函数实现

bool isValidCoord(int x, int y) {
  const Robot::Point screenSize = size();
  return x >= 0 && x < screenSize.x && y >= 0 && y < screenSize.y;
}

std::string toLower(const std::string &str) {
  std::string result = str;
  std::transform(result.begin(), result.end(), result.begin(), ::tolower);
  return result;
}

namespace ExtendedFunc {
void dragRect(const int x1, const int y1, const int width, const int height,
              double const duration, const Button button) {
  // 参数验证
  if (width <= 0 || height <= 0) {
    throw AutoGUIException("Width and height must be positive");
  }
  // 计算矩形右下角坐标
  const int x2 = x1 + width;
  const int y2 = y1 + height;

  // 拖动矩形：左上角 -> 右上角 -> 右下角 -> 左下角 -> 左上角
  drag(x1, y1, x2, y1, duration/4, button);  // 上边
  drag(x2, y1, x2, y2, duration/4, button);  // 右边
  drag(x2, y2, x1, y2, duration/4, button);  // 下边
  drag(x1, y2, x1, y1, duration/4, button);  // 左边
}

void dragCircle(const int centerX, const int centerY, const int radius,
                double const duration, const Button button) {
  // 参数验证
  if (radius <= 0) {
    throw AutoGUIException("Radius must be positive");
  }
  // 计算圆上的点数量（越多越平滑）
  const int segments = 36;  // 每10度一个点
  const double angleStep = 2.0 * M_PI / segments;
  // 从3点钟方向开始
  double startX = centerX + radius;
  double startY = centerY;

  // 拖动圆形
  for (int i = 1; i <= segments; i++) {
    double angle = angleStep * i;
    double endX = centerX + radius * cos(angle);
    double endY = centerY + radius * sin(angle);
    // 拖动到下一个点
    drag(static_cast<int>(startX), static_cast<int>(startY),
         static_cast<int>(endX), static_cast<int>(endY),
         duration / segments, button);
    // 更新起点为终点
    startX = endX;
    startY = endY;
  }
}

void dragRandomInArea(const int x1, const int y1, const int x2, const int y2,
                      const double minDuration, const double maxDuration,
                      const Button button) {
  // 参数验证
  if (x1 >= x2 || y1 >= y2) {
    throw AutoGUIException("Invalid area coordinates");
  }

  if (minDuration < 0 || maxDuration < 0 || minDuration > maxDuration) {
    throw AutoGUIException("Invalid duration range");
  }

  // 设置随机数生成器
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> distX(x1, x2);
  std::uniform_int_distribution<> distY(y1, y2);
  std::uniform_real_distribution<> distDuration(minDuration, maxDuration);

  // 生成随机起始点和结束点
  int startX = distX(gen);
  int startY = distY(gen);
  int endX = distX(gen);
  int endY = distY(gen);
  double duration = distDuration(gen);

  // 执行随机拖动
  drag(startX, startY, endX, endY, duration, button);
}

// 模拟人类打字
void typeHumanLike(const std::string& text, double minDelay, double maxDelay, double errorRate) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_real_distribution<> delayDist(minDelay, maxDelay);
  std::uniform_real_distribution<> errorDist(0.0, 1.0);

  for (char c : text) {
    // 有一定概率打错字
    if (errorDist(gen) < errorRate) {
      // 输入一个随机错误字符
      char wrongChar = 'a' + static_cast<char>(errorDist(gen) * 26);
      AutoGUI::press(std::string(1, wrongChar));

      // 退格删除错误
      AutoGUI::press("backspace");
      AutoGUI::sleep(0.1);
    }

    // 输入正确字符
    AutoGUI::press(std::string(1, c));

    // 随机延迟
    double delay = delayDist(gen);
    AutoGUI::sleep(delay);
  }
}

void typewriteEnter(const std::string& text, double interval) {
  AutoGUI::type(text, interval);
  AutoGUI::press("enter");
}

void typewriteLines(const std::vector<std::string>& lines, double interval, double lineDelay) {
  for (size_t i = 0; i < lines.size(); i++) {
    AutoGUI::type(lines[i], interval);
    if (i < lines.size() - 1) {
      AutoGUI::press("enter");
      AutoGUI::sleep(lineDelay);
    }
  }
}

void typeDateTime(const std::string& format) {
  const auto now = std::chrono::system_clock::now();
  const auto time = std::chrono::system_clock::to_time_t(now);
  const std::tm tm = *std::localtime(&time);

  std::stringstream ss;
  ss << std::put_time(&tm, format.c_str());
  AutoGUI::type(ss.str());
}

/// 键盘快捷键扩展实现
void saveFile() {
  AutoGUI::hotkey({"ctrl", "s"});
}

void openFile() {
  AutoGUI::hotkey({"ctrl", "o"});
}

void newFile() {
  AutoGUI::hotkey({"ctrl", "n"});
}

void copy() {
  AutoGUI::hotkey({"ctrl", "c"});
}

void paste() {
  AutoGUI::hotkey({"ctrl", "v"});
}

void cut() {
  AutoGUI::hotkey({"ctrl", "x"});
}

void selectAll() {
  AutoGUI::hotkey({"ctrl", "a"});
}

void undo() {
  AutoGUI::hotkey({"ctrl", "z"});
}

void redo() {
#ifdef _WIN32
  AutoGUI::hotkey({"ctrl", "y"});
#else
  AutoGUI::hotkey({"ctrl", "shift", "z"});
#endif
}

void find() {
  AutoGUI::hotkey({"ctrl", "f"});
}

void replace() {
  AutoGUI::hotkey({"ctrl", "h"});
}

void print() {
  AutoGUI::hotkey({"ctrl", "p"});
}

void closeWindow(bool useAltF4) {
  if (useAltF4) {
    AutoGUI::hotkey({"alt", "f4"});
  } else {
    AutoGUI::hotkey({"ctrl", "w"});
  }
}

void switchWindow(int times) {
  for (int i = 0; i < times; i++) {
    AutoGUI::hotkey({"alt", "tab"});
    AutoGUI::sleep(0.1);
  }
}

void refresh() {
  AutoGUI::press("f5");
}

/// 游戏/自动化操作实现
void autoClicker(int x, int y, int count, double interval, AutoGUI::Button button) {
  for (int i = 0; i < count; i++) {
    AutoGUI::click(x, y, button);
    if (i < count - 1) {
      AutoGUI::sleep(interval);
    }
  }
}

void rapidClicker(int x, int y, double duration, double clickRate, AutoGUI::Button button) {
  int totalClicks = static_cast<int>(duration * clickRate);
  double interval = 1.0 / clickRate;

  for (int i = 0; i < totalClicks; i++) {
    AutoGUI::click(x, y, button);
    AutoGUI::sleep(interval);
  }
}

void rapidKeyPress(const std::string& key, double duration, double pressRate) {
  int totalPresses = static_cast<int>(duration * pressRate);
  double interval = 1.0 / pressRate;

  for (int i = 0; i < totalPresses; i++) {
    AutoGUI::press(key);
    AutoGUI::sleep(interval);
  }
}

void rapidHotkey(const std::vector<std::string>& keys, double duration, double pressRate) {
  int totalPresses = static_cast<int>(duration * pressRate);
  double interval = 1.0 / pressRate;

  for (int i = 0; i < totalPresses; i++) {
    AutoGUI::hotkey(keys);
    AutoGUI::sleep(interval);
  }
}

} // namespace ExtendedFunc

} // namespace AutoGUI