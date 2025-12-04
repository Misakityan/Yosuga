[English](README.md) / [日本語](README.ja.md)

---

# Live2D Cubism Core

本文件夹包含用于开发原生应用程序的头文件以及各平台专用的库文件。

Cubism Core 是一个轻量级库，用于在应用程序中加载 Cubism 模型。

## 入门

若要将模型显示到屏幕上并播放动画，请参考 [CubismNativeSamples](https://github.com/Live2D/CubismNativeSamples)。

## 文件结构

```
Core
├─ dll          # 共享（动态）库文件
├─ include      # 头文件
└─ lib          # 静态库文件
```

## 库列表

| 平台 | 架构 | dll | lib | 路径 | 备注 |
| --- | --- | --- | --- | --- | --- |
| Android | ARM64 | ✓ | ✓ | android/arm64-v8a | |
| Android | x86 | ✓ | ✓ | android/x86 | |
| Android | x86_64 | ✓ | ✓ | android/x86_64 | |
| iOS | ARM64 | | ✓ | ios/xxx-iphoneos | iOS 真机 |
| iOS | x86_64 | | ✓ | ios/xxx-iphonesimulator | iOS 模拟器 |
| Linux | x86_64 | ✓ | ✓ | linux/x86_64 | |
| Linux | ARM64 | ✓ | ✓ | experimental/linux/ARM64 | |
| macOS | ARM64 | ✓ | ✓ | macos/arm64 | |
| macOS | x86_64 | ✓ | ✓ | macos/x86_64 | |
| Mac Catalyst | ARM64 | | ✓ | experimental/catalyst | Universal Binary |
| Mac Catalyst | x86_64 | | ✓ | experimental/catalyst | Universal Binary |
| Raspberry Pi | ARM | ✓ | ✓ | experimental/rpi | |
| UWP | ARM | ✓ | | experimental/uwp/arm | |
| UWP | ARM64 | ✓ | | experimental/uwp/arm64 | |
| UWP | x64 | ✓ | | experimental/uwp/x64 | |
| UWP | x86 | ✓ | | experimental/uwp/x86 | |
| Windows | x86 | ✓ | ✓ | windows/x86 | |
| Windows | x86_64 | ✓ | ✓ | windows/x86_64 | |

### 实验性库

`Raspberry Pi`、`UWP`、`Catalyst` 属于实验性库。

### Windows 静态库说明

库文件位于以 VC++ 版本命名的目录下。

各 VC++ 版本对应的 Visual Studio 版本如下：

| VC++ 版本 | Visual Studio 版本 |
| ---: | --- |
| 140 | Visual Studio 2015 |
| 141 | Visual Studio 2017 |
| 142 | Visual Studio 2019 |
| 143 | Visual Studio 2022 |

后缀库（`_MD`、`_MDd`、`_MT`、`_MTd`）表示编译该库时所使用的[运行时库](https://docs.microsoft.com/en-us/cpp/build/reference/md-mt-ld-use-run-time-library)选项。

### 调用约定

使用 *Windows/x86* 的动态链接库时，请显式采用 `__stdcall` 调用约定。