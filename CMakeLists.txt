cmake_minimum_required(VERSION 3.5)
project(Yosuga VERSION 1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(CMAKE_PREFIX_PATH "/home/misaki/Qt6.3/6.6.3/gcc_64")    # 设置Qt6安装路径(此处请根据你的Qt6安装位置填写)

# 添加ElaWidgetTools UI库项目
add_subdirectory(3rdparty/ElaWidgetTools)

# 设置是否为debug模式 默认为Debug
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

# 设置渲染方式，选择为OpenGL，不然默认是Cocos2d，编译会报错的，CMake也会给你警告的
set(FRAMEWORK_SOURCE OpenGL)

# 查找一些必要的src文件
file(GLOB_RECURSE LAppLive2D
        CONFIGURE_DEPENDS       # CMake 3.12+：检测到新增文件自动重新生成
    "3rdparty/Live2D/Src/LAppLive2D/Src/*.cpp"
)
file(GLOB_RECURSE YosugaSrc
        CONFIGURE_DEPENDS
        "src/AudioHandle/Src/*.cpp"
        "src/AudioHandle/Inc/*.h"
        "src/Menu/Src/*.cpp"
        "src/Menu/Inc/*.h"
        "src/NetWorkHandle/Src/*.cpp"
        "src/NetWorkHandle/Inc/*.h"
        "src/Setting/Src/*.cpp"
        "src/Setting/Inc/*.h"
        "src/Render/TextRender/Src/*.cpp"
        "src/Render/TextRender/Inc/*.h"
        "src/Core/Src/*.cpp"
        "src/Core/Inc/*.h"
)

# 查找Qt6模块以及其他必须模块
find_package(Qt6 COMPONENTS
        Core
        Gui
        Widgets
        Network
        Svg
        SerialPort
        WebSockets
        Multimedia
        OpenGLWidgets
        REQUIRED)
find_package(OpenGL REQUIRED)


add_executable(${PROJECT_NAME} main.cpp ${LAppLive2D} ${YosugaSrc})

# 区分平台
if(WIN32)
    set(PLAT "windows")
    add_definitions(-DCSM_TARGET_WIN_GL)    # 告诉 Live2D 框架，当前平台使用 OpenGL
elseif(APPLE)
    set(PLAT "macos")
    add_definitions(-DCSM_TARGET_MAC_GL)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|arm64")
        set(PLAT "linux_arm")
        add_definitions(-DCSM_TARGET_LINUX_GL)
    else()
        set(PLAT "linux_64")
        add_definitions(-DCSM_TARGET_LINUX_GL)
    endif()
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin" AND CMAKE_OSX_ARCHITECTURES MATCHES "arm64")
    set(PLAT "macos")   # 统一按 macos 处理，后面再细分 x86_64 / arm64
endif()

# 架构文件夹名（Windows 分 x86 / x86_64，Linux 分 x86_64 / arm64，macOS 通用二进制）
if(WIN32)
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(ARCH "x86_64")
    else()
        set(ARCH "x86")
    endif()
elseif(PLAT STREQUAL "linux_arm")
    set(ARCH "arm64")
else()
    set(ARCH "x86_64")
endif()

#### 以下为链接相关的内容

# 根据平台 + 架构生成“库搜索路径”和“库文件名”
set(LIB_BASE "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/Live2D/Lib")
# 根据当前构建类型直接确定子目录（Debug 或 Release）
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(LIB_CONFIG "Debug")
else()
    set(LIB_CONFIG "Release")
endif()

message(STATUS "当前平台与编译方式: ${PLAT} : ${ARCH} : ${LIB_CONFIG}" )

# 拼接完整库目录
set(LIB_CONFIG_DIR "${LIB_BASE}/${LIB_CONFIG}/${PLAT}")
# 需要链接的库列表
# Framework（只有 .a，且 Debug/Release 同名）
add_library(Framework STATIC IMPORTED GLOBAL)
set_target_properties(Framework PROPERTIES
        IMPORTED_LOCATION "${LIB_CONFIG_DIR}/framework/libFramework.a")
message(STATUS "当前Framework库: libFramework.a")

# glfw
add_library(glfw3 STATIC IMPORTED GLOBAL)
set_target_properties(glfw3 PROPERTIES
        IMPORTED_LOCATION "${LIB_CONFIG_DIR}/glfw/libglfw3.a")
message(STATUS "当前GLEW库: libglfw3.a")

# glew Debug/Release 同名，Debug 同时提供了 .so，此处用 .a
# Debug 带 'd' 后缀，Release 不带
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    # GLEW Unix与Windows生成行为不一致
    if(PLAT STREQUAL "windows")
        set(GLEW_LIB_NAME "libglew32d.dll.a")
    else ()   # 其他平台
        set(GLEW_LIB_NAME "libGLEWd.a")
    endif ()
elseif (CMAKE_BUILD_TYPE STREQUAL "Release")
    if (PLAT STREQUAL "windows")
        set(GLEW_LIB_NAME "libglew32.dll.a")
    else ()
        set(GLEW_LIB_NAME "libGLEW.a")
    endif ()
endif()
add_library(GLEW STATIC IMPORTED GLOBAL)
set_target_properties(GLEW PROPERTIES
        IMPORTED_LOCATION "${LIB_CONFIG_DIR}/glfw/${GLEW_LIB_NAME}")
message(STATUS "当前GLEW库: ${GLEW_LIB_NAME}")

# Live2D Cubism Core 静态
add_library(Live2DCubismCore STATIC IMPORTED GLOBAL)
if (PLAT STREQUAL "windows")
    set_target_properties(Live2DCubismCore PROPERTIES
            IMPORTED_LOCATION "${LIB_CONFIG_DIR}/live2d/static_lib/x86_64/Live2DCubismCore.lib")
    message(STATUS "当前Live2D库: Live2DCubismCore.lib")
else ()
    set_target_properties(Live2DCubismCore PROPERTIES
            IMPORTED_LOCATION "${LIB_CONFIG_DIR}/live2d/static_lib/x86_64/libLive2DCubismCore.a")
    message(STATUS "当前Live2D库: libLive2DCubismCore.a")
endif ()

target_link_libraries(${PROJECT_NAME}
        PRIVATE
        Framework
        glfw3
        GLEW
        Live2DCubismCore
        $<$<BOOL:${WIN32}>:opengl32>    # WIN32 为真时才链接
        $<$<BOOL:${WIN32}>:glu32>       # WIN32 为真时才链接
)
# 不论平台统一需要链接的库
target_link_libraries(${PROJECT_NAME}
        PRIVATE
        ElaWidgetTools
        Qt::Core
        Qt::Gui
        Qt::Widgets
        Qt::Network
        Qt::Svg
        Qt::SerialPort
        Qt::WebSockets
        Qt::Multimedia
        Qt::OpenGLWidgets
)

# 添加头文件
target_include_directories(${PROJECT_NAME}
    PRIVATE
        3rdparty/Live2D/Src/Framework/src
        3rdparty/Live2D/Src/glew/include
        3rdparty/Live2D/Src/glew/include/GL
        3rdparty/Live2D/Src/glfw/include
        3rdparty/Live2D/Src/glfw/include/GLFW
        3rdparty/Live2D/Src/Core/include
        3rdparty/Live2D/Src/stb
        3rdparty/Live2D/Src/LAppLive2D/Inc
        src/AudioHandle/Inc
        src/Menu/Inc
        src/NetWorkHandle/Inc
        src/Setting/Inc
        src/Render/TextRender/Inc
        src/Core/Inc
)



#### 构建时额外复制一些必要的文件与运行库

# 将资源文件夹 'other' 复制到构建目录
file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/other/Resources"
        DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")
# 复制 Live2D 渲染所需的 Shader 文件
file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/other/FrameworkShaders"
        DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")


# 对于windows，复制一些必要的dll库
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
        # 最简单的版本 - 只传递目录
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
elseif ()   # 其他平台
    set()
endif ()
