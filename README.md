## 📊 Project Stats

![GitHub last commit](https://img.shields.io/github/last-commit/Misakityan/Yosuga)
![GitHub issues](https://img.shields.io/github/issues/Misakityan/Yosuga)
![GitHub stars](https://img.shields.io/github/stars/Misakityan/Yosuga?style=social)

本项目使用CMake构建，基于C++Qt6以及Live2D官方SDK实现桌面宠物
(本项目由Yosuga[Qt5] 发展更新而来，项目架构与代码都有所不同，最显著的特点是本项目支持多平台)
环境为:

- cmake version 3.5

- C++ 20

- Qt6.6.3

- IDE: Clion

如何构建本项目：

1.Install latest CMake(安装最新版本的CMake) or Install Clion(或者安装Clion,更推荐安装Clion,Clion捆绑了CMake)

2.gti clone https://github.com/Misakityan/Yosuga (克隆本项目)

3.Open with Clion(使用Clion打开本项目(推荐)) or use command(或者使用命令)

```powershell
# 创建构建目录
mkdir build
cd build

# 配置 CMake ,注意此处需要指定默认生成的为mingw的makefile文件
cmake ..

# 编译项目
make
```

注意：本项目只是Yosuga的客户端部分，完整的还包括服务端
- 服务端项目地址：

当前支持平台(已测试过的)：
-    Windows:    Windows 10
-    Linux:      kUbuntu 24.04

相关教程见BiliBili：https://www.bilibili.com/video/BV1TtkHYpEDA/?spm_id_from=333.1387.homepage.video_card.click&vd_source=d66e155c7b27c10078bc67965ea1989e

实现效果：
