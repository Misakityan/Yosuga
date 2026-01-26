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

// 平台特定函数实现
Robot::Point size() {
  // 平台特定实现
#ifdef _WIN32
// Windows实现
#include <Windows.h>
  int width = GetSystemMetrics(SM_CXSCREEN);
  int height = GetSystemMetrics(SM_CYSCREEN);
  return {width, height};

#elif defined(__APPLE__)
// macOS实现
#include <CoreGraphics/CoreGraphics.h>
  CGRect mainDisplayBounds = CGDisplayBounds(CGMainDisplayID());
  int width = static_cast<int>(CGRectGetWidth(mainDisplayBounds));
  int height = static_cast<int>(CGRectGetHeight(mainDisplayBounds));
  return {width, height};

#elif defined(__linux__)
// Linux实现（X11）
#include <X11/Xlib.h>

  Display *display = XOpenDisplay(nullptr);
  if (display) {
    const Screen *screen = DefaultScreenOfDisplay(display);
    const int width = WidthOfScreen(screen);
    const int height = HeightOfScreen(screen);
    XCloseDisplay(display);
    return {width, height};
  }
  return {1920, 1080}; // 默认值

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