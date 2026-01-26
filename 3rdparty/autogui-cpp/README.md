# autogui-cpp

![GitHub last commit](https://img.shields.io/github/last-commit/Misakityan/autogui-cpp)
![GitHub issues](https://img.shields.io/github/issues/Misakityan/autogui-cpp)
![GitHub stars](https://img.shields.io/github/stars/Misakityan/autogui-cpp?style=social)

本项目继承自[Robot CPP](https://github.com/developer239/robot-cpp)，在此感谢该项目的贡献。


原先的项目仅支持`Windows`与`Mac OS`,本项目增加了对`Linux X11`的支持，`Linux Wanland`的支持正在开发中

为什么要创建本项目，这是因为我在苦苦寻找`pyautogui`的cpp实现，但很遗憾，我并没有找到这个轮子。

本项目在原项目的基础上，增加了以下文件：
1. `Autogui.cpp/.h`
2. `SimpleAutoGUI.h`

并修复了原项目的bug：
1. 模拟键盘输入的时候，无法处理大小写与符号的转换

最终实现了与`pyautogui`类似的功能。

但是，本项目不支持以下功能：
1. 模拟键盘输入只支持`ASCII`字符，如果你需要输入中文，或者其他语言的字符，这可能是一个很大的工程量。
更好的解决方案是将内容copy到剪贴板，然后粘贴到目标位置。

同时，本项目裁剪掉了原项目的部分功能：
1. `Hooks`
2. `Screen`
3. `Record`

主要是因为有着更加完美的上位替代品，例如`Screen`可用`Qt`的`QScreen`替代，并且支持的更加完美。

注意：原项目并没有说明代码的开源协议，因此关于`Robot-cpp`部分的代码，解释权归属于原作者`
developer239`。

本项目被我直接使用在[Yosuga](https://github.com/Misakityan/Yosuga)这个我自己的项目当中。

项目已在以下平台完成测试：
1. Windows 10
2. kUbuntu 24.04

由于我没有Mac PC, 因此并未在mac上进行测试，不过mac部分都是直接使用`Robot-cpp`部分的代码，原作者测试没问题，
大概也是可以使用的。如果遇到bug，尽情提issue即可。

## How to use?
想使用本项目十分容易，在你的项目CMakeLists.txt中:
```cmake
add_subdirectory(autogui-cpp)
target_link_libraries(your_target PRIVATE autogui-cpp)  # your_target为你项目的构建目标
# 注意：有时候你还需要根据导入本项目的位置，在autogui-cpp的前面增加一些路径信息
```
之后，在你需要的地方`#include "SimpleAutoGUI.h"`，访问AutoGUI当中已经封装好的函数即可使用。


## some examples
```c++
#include <iostream>
#include "SimpleAutoGUI.h"
int main() {
    Robot::Point screenSize = AutoGUI::size();
    std::cout << "屏幕尺寸: " << screenSize.x << "x" << screenSize.y << std::endl;

    // 测试1: 移动鼠标
    std::cout << "移动鼠标到屏幕中心..." << std::endl;
    int centerX = screenSize.x / 2;
    int centerY = screenSize.y / 2;
    AutoGUI::moveTo(centerX, centerY, 2);

    // 测试2: 单击
    std::cout << "左键单击..." << std::endl;
    AutoGUI::click();
    AutoGUI::sleep(0.5);

    // 测试3: 右键单击
    std::cout << "右键单击..." << std::endl;
    AutoGUI::rightSingle();
    AutoGUI::sleep(0.5);

    // 测试4: 双击
    std::cout << "左键双击..." << std::endl;
    AutoGUI::leftDouble();
    AutoGUI::sleep(0.5);

    // 测试5: 拖拽
    std::cout << "拖拽测试..." << std::endl;
    AutoGUI::dragRel(100, 100, 0.5);
    AutoGUI::sleep(0.5);

    // 测试6: 输入文本
    std::cout << "输入文本测试..." << std::endl;
    AutoGUI::type("Hello, AutoGUI!", 0.05); // 每个字符间隔50ms
    AutoGUI::sleep(0.5);

    // 测试7: 快捷键
    std::cout << "快捷键测试 (Ctrl+A)..." << std::endl;
    AutoGUI::hotkey({AutoGUI::Keys::CTRL, "a"});
    AutoGUI::sleep(0.5);

    std::cout << "快捷键测试 (Ctrl+C)..." << std::endl;
    AutoGUI::hotkey({AutoGUI::Keys::CTRL, "c"});
    AutoGUI::sleep(0.5);

    // 测试8: 滚动
    std::cout << "向上滚动..." << std::endl;
    AutoGUI::scroll(5);
    AutoGUI::sleep(0.5);
    
    std::cout << "向下滚动..." << std::endl;
    AutoGUI::scroll(-5);
    AutoGUI::sleep(0.5);

    // 测试9: 获取鼠标位置
    Robot::Point pos = AutoGUI::position();
    std::cout << "当前鼠标位置: (" << pos.x << ", " << pos.y << ")" << std::endl;
    return 0;
}
```