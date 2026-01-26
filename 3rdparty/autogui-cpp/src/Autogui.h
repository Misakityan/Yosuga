//
// Created by misaki on 2026/1/25.
//
#pragma once

#include <algorithm>
#include <string>
#include <utility>
#include <vector>
#include <map>
#include <initializer_list>

#include "Keyboard.h"
#include "Mouse.h"
#include "types.h"

namespace AutoGUI {

// 错误类型
class AutoGUIException : public std::exception {
private:
    std::string message;

public:
    explicit AutoGUIException(std::string  msg) : message(std::move(msg)) {}

    [[nodiscard]] const char* what() const noexcept override {
        return message.c_str();
    }
};

// 鼠标按钮枚举
enum class Button {
    LEFT = 0,
    RIGHT = 1,
    MIDDLE = 2
};

// 将 Button 转换为 Robot::MouseButton
inline Robot::MouseButton toRobotButton(Button button) {
    switch (button) {
        case Button::LEFT: return Robot::MouseButton::LEFT_BUTTON;
        case Button::RIGHT: return Robot::MouseButton::RIGHT_BUTTON;
        case Button::MIDDLE: return Robot::MouseButton::CENTER_BUTTON;
        default: return Robot::MouseButton::LEFT_BUTTON;
    }
}

// 将 Button 枚举转换为字符串
inline std::string buttonToString(Button button) {
    switch (button) {
        case Button::LEFT: return "left";
        case Button::RIGHT: return "right";
        case Button::MIDDLE: return "middle";
        default: return "left";
    }
}

// 键名字符串到 Keyboard::SpecialKey 的映射
inline Robot::Keyboard::SpecialKey stringToSpecialKey(const std::string& key) {
    static std::map<std::string, Robot::Keyboard::SpecialKey> keyMap = {
        {"backspace", Robot::Keyboard::BACKSPACE},
        {"enter", Robot::Keyboard::ENTER},
        {"return", Robot::Keyboard::ENTER},
        {"tab", Robot::Keyboard::TAB},
        {"escape", Robot::Keyboard::ESCAPE},
        {"esc", Robot::Keyboard::ESCAPE},
        {"up", Robot::Keyboard::UP},
        {"down", Robot::Keyboard::DOWN},
        {"right", Robot::Keyboard::RIGHT},
        {"left", Robot::Keyboard::LEFT},
        {"win", Robot::Keyboard::META},
        {"command", Robot::Keyboard::META},
        {"cmd", Robot::Keyboard::META},
        {"alt", Robot::Keyboard::ALT},
        {"ctrl", Robot::Keyboard::CONTROL},
        {"control", Robot::Keyboard::CONTROL},
        {"shift", Robot::Keyboard::SHIFT},
        {"capslock", Robot::Keyboard::CAPSLOCK},
        {"f1", Robot::Keyboard::F1},
        {"f2", Robot::Keyboard::F2},
        {"f3", Robot::Keyboard::F3},
        {"f4", Robot::Keyboard::F4},
        {"f5", Robot::Keyboard::F5},
        {"f6", Robot::Keyboard::F6},
        {"f7", Robot::Keyboard::F7},
        {"f8", Robot::Keyboard::F8},
        {"f9", Robot::Keyboard::F9},
        {"f10", Robot::Keyboard::F10},
        {"f11", Robot::Keyboard::F11},
        {"f12", Robot::Keyboard::F12},
        {"space", Robot::Keyboard::BACKSPACE}   // 特殊处理空格
    };

    std::string lower = key;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

    auto it = keyMap.find(lower);
    if (it != keyMap.end()) {
        return it->second;
    }

    // 如果是单个字符，直接返回（会在其他地方处理）
    if (lower.length() == 1) {
        // 这是单个字符键，不是特殊键
        throw AutoGUIException("Unknown key: " + key);
    }

    throw AutoGUIException("Unknown key: " + key);
}

// 检查字符串是否是特殊键
inline bool isSpecialKey(const std::string& key) {
    static std::vector<std::string> specialKeys = {
        "backspace", "enter", "return", "tab", "escape", "esc",
        "up", "down", "right", "left", "win", "command", "cmd",
        "alt", "ctrl", "control", "shift", "capslock",
        "f1", "f2", "f3", "f4", "f5", "f6", "f7", "f8", "f9", "f10", "f11", "f12",
        "space"
    };

    std::string lower = key;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

    return std::find(specialKeys.begin(), specialKeys.end(), lower) != specialKeys.end();
}

// 主要 API 函数
/**
 * @brief 移动鼠标到指定位置
 * @param x 目标位置的X坐标
 * @param y 目标位置的Y坐标
 * @param duration 移动持续时间（秒），0表示立即移动
 */
void moveTo(int x, int y, double duration = 0.0);

/**
 * @brief 相对移动鼠标
 * @param xOffset X方向偏移量
 * @param yOffset Y方向偏移量
 * @param duration 移动持续时间（秒），0表示立即移动
 */
void moveRel(int xOffset, int yOffset, double duration = 0.0);

/**
 * @brief 单击鼠标
 * @param x 点击位置的X坐标，-1表示当前位置
 * @param y 点击位置的Y坐标，-1表示当前位置
 * @param button 鼠标按钮：left, right, middle
 * @param clicks 点击次数
 * @param interval 多次点击之间的间隔（秒）
 */
void click(int x = -1, int y = -1,
           Button button = Button::LEFT,
           int clicks = 1,
           double interval = 0.0);

/**
 * @brief 左键双击
 * @param x 双击位置的X坐标，-1表示当前位置
 * @param y 双击位置的Y坐标，-1表示当前位置
 */
void leftDouble(int x = -1, int y = -1);

/**
 * @brief 右键单击
 * @param x 点击位置的X坐标，-1表示当前位置
 * @param y 点击位置的Y坐标，-1表示当前位置
 */
void rightSingle(int x = -1, int y = -1);

/**
 * @brief 中键单击
 * @param x 点击位置的X坐标，-1表示当前位置
 * @param y 点击位置的Y坐标，-1表示当前位置
 */
void middleClick(int x = -1, int y = -1);

/**
 * @brief 按下鼠标按钮
 * @param button 要按下的鼠标按钮
 * @param x 按下位置的X坐标，-1表示当前位置
 * @param y 按下位置的Y坐标，-1表示当前位置
 */
void mouseDown(Button button = Button::LEFT, int x = -1, int y = -1);

/**
 * @brief 释放鼠标按钮
 * @param button 要释放的鼠标按钮
 * @param x 释放位置的X坐标，-1表示当前位置
 * @param y 释放位置的Y坐标，-1表示当前位置
 */
void mouseUp(Button button = Button::LEFT, int x = -1, int y = -1);

/**
 * @brief 从指定位置拖动到另一个位置
 * @param x1 起始位置的X坐标
 * @param y1 起始位置的Y坐标
 * @param x2 目标位置的X坐标
 * @param y2 目标位置的Y坐标
 * @param duration 拖动持续时间（秒），0表示立即拖动
 * @param button 拖动时按住的鼠标按钮
 */
void drag(int x1, int y1, int x2, int y2,
          double duration = 0.0,
          Button button = Button::LEFT);

/**
 * @brief 拖拽鼠标到指定位置
 * @param x 目标位置的X坐标
 * @param y 目标位置的Y坐标
 * @param duration 拖拽持续时间（秒），0表示立即拖拽
 * @param button 拖拽时按住的鼠标按钮
 */
void dragTo(int x, int y, double duration = 0.0, Button button = Button::LEFT);

/**
 * @brief 相对拖拽鼠标
 * @param xOffset X方向偏移量
 * @param yOffset Y方向偏移量
 * @param duration 拖拽持续时间（秒），0表示立即拖拽
 * @param button 拖拽时按住的鼠标按钮
 */
void dragRel(int xOffset, int yOffset, double duration = 0.0, Button button = Button::LEFT);

/**
 * @brief 获取当前鼠标位置
 * @return 包含x,y坐标的Point结构体
 */
Robot::Point position();

/**
 * @brief 滚动鼠标滚轮
 * @param clicks 滚动量，正数向上滚动，负数向下滚动
 * @param x 水平滚动量（Linux/macOS支持）
 */
void scroll(int clicks, int x = 0);

/**
 * @brief 输入文本
 * @param text 要输入的文本
 * @param interval 字符之间的间隔时间（秒），0表示无间隔
 * 注意：只支持ASCII字符
 */
void type(const std::string& text, double interval = 0.0);

/**
 * @brief 按下并释放一个键
 * @param key 键名（如："a", "enter", "ctrl"等）
 */
void press(const std::string& key);

/**
 * @brief 按下键（不释放）
 * @param key 键名
 */
void keyDown(const std::string& key);

/**
 * @brief 释放键
 * @param key 键名
 */
void keyUp(const std::string& key);

/**
 * @brief 按下组合键
 * @param keys 键名列表，如 {"ctrl", "c"}
 */
void hotkey(const std::initializer_list<std::string>& keys);

/**
 * @brief 按下组合键（向量版本）
 * @param keys 键名向量
 */
void hotkey(const std::vector<std::string>& keys);

/**
 * @brief 睡眠/等待
 * @param seconds 等待的秒数
 */
void sleep(double seconds);

/**
 * @brief 获取屏幕尺寸
 * @return 包含屏幕宽度和高度的Point结构体
 * @note 需要平台特定实现
 * @note 注意：不提供屏幕管理功能，如果你有多个屏幕，那么返回的size可能是错误的，对于单个屏幕是没有影响的，对于获取屏幕尺寸，更加推荐使用Qt当中的接口
 */
Robot::Point size();

// 辅助函数
/**
 * @brief 检查坐标是否有效
 * @param x X坐标
 * @param y Y坐标
 * @return 如果坐标有效返回true
 */
bool isValidCoord(int x, int y);

/**
 * @brief 将字符串转换为小写
 * @param str 输入字符串
 * @return 小写字符串
 */
std::string toLower(const std::string& str);

namespace ExtendedFunc {
/// drag拓展功能
/**
* @brief 从指定位置开始拖动一个矩形区域
* @param x1 矩形左上角X坐标
* @param y1 矩形左上角Y坐标
* @param width 矩形宽度
* @param height 矩形高度
* @param duration 拖动持续时间（秒）
* @param button 拖动时按住的鼠标按钮
*/
void dragRect(int x1, int y1, int width, int height,
              double duration = 0.5, Button button = Button::LEFT);

/**
 * @brief 拖动一个圆形区域（从中心开始到边缘）
 * @param centerX 圆心X坐标
 * @param centerY 圆心Y坐标
 * @param radius 圆半径
 * @param duration 拖动持续时间（秒）
 * @param button 拖动时按住的鼠标按钮
 */
void dragCircle(int centerX, int centerY, int radius,
                double duration = 0.5, Button button = Button::LEFT);

/**
 * @brief 在区域内随机拖动（用于测试或自动化）
 * @param x1 区域左上角X坐标
 * @param y1 区域左上角Y坐标
 * @param x2 区域右下角X坐标
 * @param y2 区域右下角Y坐标
 * @param minDuration 最小拖动持续时间（秒）
 * @param maxDuration 最大拖动持续时间（秒）
 * @param button 拖动时按住的鼠标按钮
 */
void dragRandomInArea(int x1, int y1, int x2, int y2,
                      double minDuration = 0.1, double maxDuration = 0.5,
                      Button button = Button::LEFT);

/// 文字输入扩展功能
/**
 * @brief 模拟人类打字（带随机延迟和可能的错误）
 * @param text 要输入的文本
 * @param minDelay 最小延迟（秒）
 * @param maxDelay 最大延迟（秒）
 * @param errorRate 错误率（0.0-1.0）
 */
void typeHumanLike(const std::string& text, double minDelay = 0.05,
                   double maxDelay = 0.2, double errorRate = 0.0);

/**
 * @brief 输入文本并回车
 * @param text 要输入的文本
 * @param interval 字符间隔
 */
void typewriteEnter(const std::string& text, double interval = 0.0);

/**
 * @brief 输入多行文本
 * @param lines 文本行数组
 * @param interval 字符间隔
 * @param lineDelay 行间延迟
 */
void typewriteLines(const std::vector<std::string>& lines,
                    double interval = 0.0, double lineDelay = 0.1);

/**
 * @brief 输入当前日期时间
 * @param format 时间格式（如："%Y-%m-%d %H:%M:%S"）
 */
void typeDateTime(const std::string& format = "%Y-%m-%d %H:%M:%S");

/// 键盘快捷键扩展功能
/**
 * @brief 保存文件 (Ctrl+S)
 */
void saveFile();

/**
 * @brief 打开文件 (Ctrl+O)
 */
void openFile();

/**
 * @brief 新建文件 (Ctrl+N)
 */
void newFile();

/**
 * @brief 复制 (Ctrl+C)
 */
void copy();

/**
 * @brief 粘贴 (Ctrl+V)
 */
void paste();

/**
 * @brief 剪切 (Ctrl+X)
 */
void cut();

/**
 * @brief 全选 (Ctrl+A)
 */
void selectAll();

/**
 * @brief 撤销 (Ctrl+Z)
 */
void undo();

/**
 * @brief 重做 (Ctrl+Y 或 Ctrl+Shift+Z)
 */
void redo();

/**
 * @brief 查找 (Ctrl+F)
 */
void find();

/**
 * @brief 替换 (Ctrl+H)
 */
void replace();

/**
 * @brief 打印 (Ctrl+P)
 */
void print();

/**
 * @brief 关闭窗口 (Alt+F4 或 Ctrl+W)
 */
void closeWindow(bool useAltF4 = true);

/**
 * @brief 切换窗口 (Alt+Tab)
 * @param times 切换次数
 */
void switchWindow(int times = 1);

/**
 * @brief 刷新 (F5)
 */
void refresh();

/// 游戏/自动化操作功能拓展
/**
 * @brief 自动点击器（连续点击）
 * @param x 点击位置X坐标
 * @param y 点击位置Y坐标
 * @param count 点击次数
 * @param interval 点击间隔（秒）
 * @param button 鼠标按钮
 */
void autoClicker(int x, int y, int count = 10, double interval = 0.1,
                 Button button = Button::LEFT);

/**
 * @brief 连点器（按住连点）
 * @param x 点击位置X坐标
 * @param y 点击位置Y坐标
 * @param duration 持续时间（秒）
 * @param clickRate 点击频率（次/秒）
 * @param button 鼠标按钮
 */
void rapidClicker(int x, int y, double duration = 1.0,
                  double clickRate = 10.0,
                  Button button = Button::LEFT);

/**
 * @brief 按键连发（按住按键）
 * @param key 按键名称
 * @param duration 持续时间（秒）
 * @param pressRate 按键频率（次/秒）
 */
void rapidKeyPress(const std::string& key, double duration = 1.0,
                   double pressRate = 10.0);

/**
 * @brief 组合键连发
 * @param keys 按键组合
 * @param duration 持续时间
 * @param pressRate 按键频率
 */
void rapidHotkey(const std::vector<std::string>& keys, double duration = 1.0,
                 double pressRate = 5.0);

} // namespace ExtendedFunc

} // namespace AutoGUI
