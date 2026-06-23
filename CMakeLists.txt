cmake_minimum_required(VERSION 3.30)
project(Yosuga VERSION 1.0)

set(CMAKE_CXX_STANDARD 20)      # C++20
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# 平台与架构检测
if(DEFINED TARGET_ARCH AND TARGET_ARCH STREQUAL "arm64")
    set(PLAT "linux_arm")
    set(ARCH "arm64")
    set(CMAKE_PREFIX_PATH "/home/misaki/MisakiCodes/rk3566-sdk/3rd/Qt6.6.3")
    add_definitions(-DCSM_TARGET_HARMONYOS_ES3)   # OpenGL ES 3.0 渲染宏
    add_definitions(-DEMBEDDED_LINUX)               # 嵌入式Linux专用宏
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|arm64|ARM64")
    set(PLAT "linux_arm")
    set(ARCH "arm64")
    set(CMAKE_PREFIX_PATH "/home/misaki/MisakiCodes/rk3566-sdk/3rd/Qt6.6.3")
    add_definitions(-DCSM_TARGET_HARMONYOS_ES3)
    add_definitions(-DEMBEDDED_LINUX)
elseif(WIN32)
    set(PLAT "windows")
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(ARCH "x86_64")
    else()
        set(ARCH "x86")
    endif()
    set(CMAKE_PREFIX_PATH "Qt/6.6.3/gcc_64")        ## 按照自己电脑上qt6.6.3的位置来替换
    add_definitions(-DCSM_TARGET_WIN_GL)
elseif(APPLE)
    set(PLAT "macos")
    set(ARCH "x86_64")   # 或通用二进制，可根据需要调整
    set(CMAKE_PREFIX_PATH "/home/misaki/Qt/6.6.3/gcc_64")
    add_definitions(-DCSM_TARGET_MAC_GL)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(PLAT "linux_64")
    set(ARCH "x86_64")
    set(CMAKE_PREFIX_PATH "/home/misaki/Qt6.3/6.6.3/gcc_64")
    add_definitions(-DCSM_TARGET_LINUX_GL)
else()
    message(FATAL_ERROR "Unsupported platform")
endif()

# 渲染后端选择
if(PLAT STREQUAL "linux_arm")
    add_definitions(-DRENDER_BACKEND_GLES2)
else()
    add_definitions(-DRENDER_BACKEND_OPENGL)
endif()

message(STATUS "  当前Qt路径: ${CMAKE_PREFIX_PATH}")
message(STATUS "  当前平台： ${PLAT}， 架构： ${ARCH}")

# 设置是否为debug模式 默认为Debug
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

# 设置渲染方式，选择为OpenGL，不然默认是Cocos2d，编译会报错的，CMake也会给你警告的
set(FRAMEWORK_SOURCE OpenGL)

# 查找一些必要的src源文件
file(GLOB_RECURSE YosugaSrc
        CONFIGURE_DEPENDS
        "src/Handle/AudioHandle/Src/*.cpp"
        "src/Handle/AudioHandle/Inc/*.h"
        "src/Handle/NetWorkHandle/Src/*.cpp"
        "src/Handle/NetWorkHandle/Inc/*.h"
        "src/Handle/DataObjectHandle/Src/*.cpp"
        "src/Handle/DataObjectHandle/Inc/*.h"
        "src/DAO/Inc/*.h"
        "src/DAO/Src/*.cpp"
        "src/UI/Render/TextRender/Src/*.cpp"
        "src/UI/Render/TextRender/Inc/*.h"
        "src/Core/Src/*.cpp"
        "src/Core/Inc/*.h"
        "src/Utils/Inc/*.hpp"
        "src/Utils/Inc/*.h"
        "src/Utils/Src/*.cpp"
)

# 如果是桌面平台，再额外添加 UI 相关目录
if(NOT PLAT STREQUAL "linux_arm")
    file(GLOB_RECURSE DesktopUI_Src
            "src/UI/Menu/Src/*.cpp"
            "src/UI/Menu/Inc/*.h"
            "src/UI/Setting/Src/*.cpp"
            "src/UI/Setting/Inc/*.h"
    )
    message(STATUS " 被主机平台额外包含的库： ${YosugaSrc}")
    list(APPEND YosugaSrc ${DesktopUI_Src})
endif()

# 查找Qt6模块以及其他必须模块
find_package(Qt6 COMPONENTS
        Core
        Gui
        Widgets
        Network             # 网络
        Svg                 # svg
        SerialPort          # 串口
        WebSockets          # websocket
        Multimedia          # 音频
        OpenGLWidgets
        Concurrent
        REQUIRED)

# 仅在非 ARM 平台查找桌面 OpenGL
if(NOT PLAT STREQUAL "linux_arm")
    find_package(OpenGL REQUIRED)
else ()
    # 若为arm平台，对于C++采用静态链接标准库，避免环境复杂出现问题
    set(CMAKE_EXE_LINKER_FLAGS "-static-libstdc++ -static-libgcc")
endif ()

# 添加第三方子项目
if(NOT PLAT STREQUAL "linux_arm")
    ### 部分第三方子项目与嵌入式Linux不兼容(嵌入式Linux环境复杂，有的没有桌面环境，因此不一定支持窗口)
    add_subdirectory(3rdparty/ElaWidgetTools)   # 添加ElaWidgetTools UI库
    add_subdirectory(3rdparty/autogui-cpp)      # 添加autogui-cpp GUI自动化库
else()
    message(STATUS "ARM platform: skipping ElaWidgetTools and autogui-cpp (cross‑compile them separately)")
endif()

#### 主可执行文件
add_executable(${PROJECT_NAME} main.cpp ${YosugaSrc})

#### 库导入路径配置
# Framework 静态库
if(PLAT STREQUAL "linux_arm")
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        ### 注意此处需要根据你的framework的放置目录进行调整
        set(FRAMEWORK_LIB_PATH "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/Live2D/Lib/Debug/linux_arm/framework/misaki_rk3566/libFramework.a")
    else()
        set(FRAMEWORK_LIB_PATH "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/Live2D/Lib/Release/linux_arm/framework/misaki_rk3566/libFramework.a")
    endif()
else()
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(LIB_CONFIG "Debug")
    else()
        set(LIB_CONFIG "Release")
    endif()
    set(LIB_BASE "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/Live2D/Lib")
    set(LIB_CONFIG_DIR "${LIB_BASE}/${LIB_CONFIG}/${PLAT}")
    set(FRAMEWORK_LIB_PATH "${LIB_CONFIG_DIR}/framework/libFramework.a")
endif()

add_library(Framework STATIC IMPORTED GLOBAL)
set_target_properties(Framework PROPERTIES IMPORTED_LOCATION "${FRAMEWORK_LIB_PATH}")
message(STATUS "Framework 库: ${FRAMEWORK_LIB_PATH}")

# Core 静态库 该库由官方自行编译提供，理论上是平台无关的，但是可能和glibc版本有关，太老的glibc版本不一定能使用
# 因为Core没有开源，也因此才叫Live2D SDK,而不是Live2D Lib。个人觉得不如开源出来，像Qt那样就挺好。
if(PLAT STREQUAL "linux_arm")
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(CORE_LIB_PATH "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/Live2D/Lib/Debug/linux_arm/live2d/static_lib/arm64/libLive2DCubismCore.a")
    else()
        set(CORE_LIB_PATH "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/Live2D/Lib/Release/linux_arm/live2d/static_lib/arm64/libLive2DCubismCore.a")
    endif()
else()
    if(PLAT STREQUAL "windows")
        set(CORE_LIB_PATH "${LIB_CONFIG_DIR}/live2d/static_lib/x86_64/Live2DCubismCore.lib")
    else()
        set(CORE_LIB_PATH "${LIB_CONFIG_DIR}/live2d/static_lib/x86_64/libLive2DCubismCore.a")
    endif()
endif()

add_library(Live2DCubismCore STATIC IMPORTED GLOBAL)
set_target_properties(Live2DCubismCore PROPERTIES IMPORTED_LOCATION "${CORE_LIB_PATH}")
message(STATUS "Live2D Core 库: ${CORE_LIB_PATH}")

# 导入 LAppLive2D 子项目 Live2D 应用层，静态库
# 依赖顺序：lapp_live2d -> Framework -> Live2DCubismCore
add_subdirectory(3rdparty/Live2D/Src/LAppLive2D)

# LAppOpenGL.hpp 根据平台选择 GL / GLES2 头文件，为 lapp_live2d 补充对应路径
if(NOT PLAT STREQUAL "linux_arm")
    target_include_directories(lapp_live2d PUBLIC
            3rdparty/Live2D/Src/glew/include
            3rdparty/Live2D/Src/glew/include/GL
            3rdparty/Live2D/Src/glfw/include
            3rdparty/Live2D/Src/glfw/include/GLFW
    )
else()
    target_include_directories(lapp_live2d PUBLIC
            /home/misaki/MisakiCodes/rk3566-sdk/host/aarch64-buildroot-linux-gnu/sysroot/usr/include
            /home/misaki/MisakiCodes/rk3566-sdk/host/aarch64-buildroot-linux-gnu/sysroot/usr/include/GLES2
    )
endif()

# GLFW 和 GLEW（仅桌面平台需要）
if(NOT PLAT STREQUAL "linux_arm")
    # glfw
    add_library(glfw3 STATIC IMPORTED GLOBAL)
    set_target_properties(glfw3 PROPERTIES IMPORTED_LOCATION "${LIB_CONFIG_DIR}/glfw/libglfw3.a")
    message(STATUS "GLFW 库: libglfw3.a")

    # glew
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        if(PLAT STREQUAL "windows")
            set(GLEW_LIB_NAME "libglew32d.dll.a")
        else()
            set(GLEW_LIB_NAME "libGLEWd.a")
        endif()
    else()
        if(PLAT STREQUAL "windows")
            set(GLEW_LIB_NAME "libglew32.dll.a")
        else()
            set(GLEW_LIB_NAME "libGLEW.a")
        endif()
    endif()
    add_library(GLEW STATIC IMPORTED GLOBAL)
    set_target_properties(GLEW PROPERTIES IMPORTED_LOCATION "${LIB_CONFIG_DIR}/glfw/${GLEW_LIB_NAME}")
    message(STATUS "GLEW 库: ${GLEW_LIB_NAME}")
endif()

#### 以下为链接相关的内容
if(PLAT STREQUAL "linux_arm")       ## 针对嵌入式Linux的配置，部分条目需要根据你的soc以及sysroot进行修改
    target_link_libraries(${PROJECT_NAME} PRIVATE
            lapp_live2d
            Framework
            Live2DCubismCore
            # Mali GPU 驱动完整链接（EGLFS 必需）
            -lmali-hook     # 注意此处，你的soc不一定是该GPU，可能需要修改
            -Wl,--whole-archive -lmali-hook-injector -Wl,--no-whole-archive
            -lmali
            -ldrm
            -lEGL
            -lGLESv2
            # Qt 模块（从交叉编译的 Qt）
            Qt::Core Qt::Gui Qt::Widgets Qt::Network Qt::Svg Qt::SerialPort
            Qt::WebSockets Qt::Multimedia Qt::OpenGLWidgets Qt::Concurrent
            #
            # ElaWidgetTools autogui-cpp
    )
else()
    # 桌面平台
    target_link_libraries(${PROJECT_NAME} PRIVATE
            lapp_live2d
            Framework
            glfw3
            GLEW
            Live2DCubismCore
            $<$<BOOL:${WIN32}>:opengl32>
            $<$<BOOL:${WIN32}>:glu32>
            ElaWidgetTools
            autogui-cpp
            Qt::Core Qt::Gui Qt::Widgets Qt::Network Qt::Svg Qt::SerialPort
            Qt::WebSockets Qt::Multimedia Qt::OpenGLWidgets Qt::Concurrent
    )
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT PLAT STREQUAL "linux_arm")
        find_package(X11 REQUIRED)
        target_link_libraries(${PROJECT_NAME} PRIVATE X11::X11)
    endif()
    if(APPLE)
        find_library(COREGRAPHICS_LIB CoreGraphics REQUIRED)
        target_link_libraries(${PROJECT_NAME} PRIVATE ${COREGRAPHICS_LIB})
    endif()
endif()

# 头文件路径
target_include_directories(${PROJECT_NAME} PRIVATE
        3rdparty/autogui-cpp/src
        src/Handle/AudioHandle/Inc
        src/Handle/NetWorkHandle/Inc
        src/Handle/DataObjectHandle/Inc
        src/UI/Menu/Inc
        src/UI/Setting/Inc
        src/UI/Render/TextRender/Inc
        src/Core/Inc
        src/DAO/Inc
        src/Utils/Inc
)

# 资源复制（所有平台）
file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/other/Resources"
        DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")
file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/other/FrameworkShaders"
        DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")

# Windows 特有的 DLL 复制
if(PLAT STREQUAL "windows")
    # 自动复制Ela的DLL
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy
            $<TARGET_FILE:ElaWidgetTools>
            $<TARGET_FILE_DIR:${PROJECT_NAME}>
    )
    # 部署 MinGW 运行时库
    if (MINGW)
        get_filename_component(MINGW_BIN_DIR ${CMAKE_CXX_COMPILER} DIRECTORY)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy
                "${MINGW_BIN_DIR}/libgcc_s_seh-1.dll"
                "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
                COMMAND ${CMAKE_COMMAND} -E copy
                "${MINGW_BIN_DIR}/libstdc++-6.dll"
                "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
                COMMAND ${CMAKE_COMMAND} -E copy
                "${MINGW_BIN_DIR}/libwinpthread-1.dll"
                "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
        )
    endif()

    # 获取windeployqt路径
    get_target_property(QT_QMAKE_EXECUTABLE Qt6::qmake IMPORTED_LOCATION)
    get_filename_component(QT_BIN_DIR "${QT_QMAKE_EXECUTABLE}" DIRECTORY)
    set(WINDEPLOYQT_EXE "${QT_BIN_DIR}/windeployqt.exe")

    # 检查windeployqt是否存在
    if(EXISTS "${WINDEPLOYQT_EXE}")
        message(STATUS "Found windeployqt: ${WINDEPLOYQT_EXE}")

        # 构建windeployqt命令
        set(WINDEPLOYQT_CMD "${WINDEPLOYQT_EXE}")

        # 添加构建类型参数
        if(CMAKE_BUILD_TYPE STREQUAL "Debug")
            set(WINDEPLOYQT_CMD "${WINDEPLOYQT_CMD} --debug")
        else()
            set(WINDEPLOYQT_CMD "${WINDEPLOYQT_CMD} --release")
        endif()

        # 添加其他参数
        set(WINDEPLOYQT_CMD "${WINDEPLOYQT_CMD} --no-compiler-runtime")
        set(WINDEPLOYQT_CMD "${WINDEPLOYQT_CMD} --no-angle")
        set(WINDEPLOYQT_CMD "${WINDEPLOYQT_CMD} --no-opengl-sw")

        # 添加目标目录
        set(WINDEPLOYQT_CMD "${WINDEPLOYQT_CMD} \"$<TARGET_FILE_DIR:${PROJECT_NAME}>\"")

        # 添加自定义命令运行windeployqt
        # 只传递目录
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND "${WINDEPLOYQT_EXE}" "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
                COMMENT "Running windeployqt to deploy Qt dependencies..."
        )
    else()
        message(WARNING "windeployqt not found at: ${WINDEPLOYQT_EXE}")
        message(WARNING "Qt dependencies will not be automatically deployed.")
    endif()

    if (CMAKE_BUILD_TYPE STREQUAL "Debug")
        file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/Live2D/Lib/Debug/windows/glfw/glew32d.dll"
                DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")
        file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/Live2D/Lib/Debug/windows/live2d/static_lib/x86_64/Live2DCubismCore.dll"
                DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")
    elseif (CMAKE_BUILD_TYPE STREQUAL "Release")
        file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/Live2D/Lib/Release/windows/glfw/glew32.dll"
                DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")
        file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/Live2D/Lib/Debug/windows/live2d/static_lib/x86_64/Live2DCubismCore.dll"
                DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")
    endif ()
endif()
