//
// Created by misaki on 2026/1/25.
//
#pragma once

// 包含所有必要头文件
#include "Autogui.h"

// 常用常量
namespace AutoGUI {
    // 鼠标按钮常量
    const Button LEFT = Button::LEFT;
    const Button RIGHT = Button::RIGHT;
    const Button MIDDLE = Button::MIDDLE;

    // 常用键常量（字符串形式，用于press、keyDown等函数）
    namespace Keys {
        const std::string BACKSPACE = "backspace";
        const std::string ENTER = "enter";
        const std::string TAB = "tab";
        const std::string ESCAPE = "escape";
        const std::string ESC = "esc";
        const std::string UP = "up";
        const std::string DOWN = "down";
        const std::string RIGHT = "right";
        const std::string LEFT = "left";
        const std::string WIN = "win";
        const std::string COMMAND = "command";
        const std::string ALT = "alt";
        const std::string CTRL = "ctrl";
        const std::string SHIFT = "shift";
        const std::string CAPSLOCK = "capslock";
        const std::string F1 = "f1";
        const std::string F2 = "f2";
        const std::string F3 = "f3";
        const std::string F4 = "f4";
        const std::string F5 = "f5";
        const std::string F6 = "f6";
        const std::string F7 = "f7";
        const std::string F8 = "f8";
        const std::string F9 = "f9";
        const std::string F10 = "f10";
        const std::string F11 = "f11";
        const std::string F12 = "f12";
        const std::string SPACE = "space";
    }
}