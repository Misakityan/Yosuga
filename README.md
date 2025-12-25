## 📊 Project Stats

![GitHub last commit](https://img.shields.io/github/last-commit/Misakityan/Yosuga)
![GitHub issues](https://img.shields.io/github/issues/Misakityan/Yosuga)
![GitHub stars](https://img.shields.io/github/stars/Misakityan/Yosuga?style=social)

本项目使用CMake构建，基于C++Qt6.6.3以及Live2D官方SDK(CubismSdkForNative-5-r.4.1)实现Live2D桌面宠物
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
- 服务端项目地址见：

当前支持平台(已测试过的)：
-    Windows:    Windows 10
-    Linux:      kUbuntu 24.04

相关教程见BiliBili：https://www.bilibili.com/video/BV1TtkHYpEDA/?spm_id_from=333.1387.homepage.video_card.click&vd_source=d66e155c7b27c10078bc67965ea1989e

实现效果：


关于授权 <br>
本项目采用多重授权结构：

1. 原创代码部分：MIT License
   src/*.*

2. 依赖库：
    - Qt 6.6.3：LGPLv3 License
      （提供源码获取方式：https://www.qt.io/download）
       Qt部分采用动态链接方式
    - Live2D Cubism SDK：Live2D Proprietary Software License
      （需遵守销售额限制及授权协议）
      Live2D部分采用静态链接 + 动态链接方式
      本应用程序包含由Live2D Inc.开发的Live2D Cubism SDK，其版权由Live2D Inc.持有。
      如果本应用程序被用作业务的主要元素*，并且其直接或间接产生的年销售额超过2000万日元，您需与Live2D Inc.签订单独的出版许可协议并支付许可费。
      此外，当您的年销售额超过2000万日元时，请您尽快与我们联系。
      请注意，如果您违反该条款，您将超出本应用程序允许的使用范围，这会造成对Live2D Inc.的知识产权侵犯，可能会导致公司的法律索赔。
      *本应用程序作为业务的主要元素使用时，包括但不限于虚拟主播的直播业务。
      这不包括应用软件用于发布产品宣传视频的情况。
3. 整体项目：受上述所有许可证约束