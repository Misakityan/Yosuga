cmake_minimum_required(VERSION 3.10)
project(autogui-cpp VERSION 1.0.0 LANGUAGES CXX)

# 基础配置
set(CMAKE_CXX_STANDARD 17)      # Cpp17
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 创建库目标
add_library(autogui-cpp STATIC
        src/Keyboard.cpp
        src/Mouse.cpp
        src/Utils.cpp
        src/Autogui.cpp
)

# 设置头文件搜索路径
# 使用生成器表达式支持add_subdirectory和install两种使用方式
target_include_directories(autogui-cpp PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
        $<INSTALL_INTERFACE:include/autogui-cpp>
)

# 平台特定配置
# macOS平台
if(APPLE)
    # 查找并链接ApplicationServices框架
    # 该框架包含CoreGraphics和Carbon相关API
    find_library(APPLICATIONSERVICES_LIBRARY ApplicationServices REQUIRED)
    target_link_libraries(autogui-cpp PUBLIC ${APPLICATIONSERVICES_LIBRARY})

# Windows平台
elseif(WIN32)
    # 链接User32.lib (提供SendInput, GetCursorPos等API)
    target_link_libraries(autogui-cpp PUBLIC User32)

# Linux/Unix平台
elseif(UNIX AND NOT APPLE)
    # Linux需要X11后端(Wayland尚未支持)
    # Linux需要X11后端
    find_package(X11 REQUIRED)
    # 查找Xtst库（XTest扩展）
    find_library(X11_Xtst_LIB Xtst)
    if(NOT X11_Xtst_LIB)
        message(FATAL_ERROR "Xtst library not found. Install libxtst-dev.")
    endif()
    # 查找Xext库（X扩展）
    find_library(X11_Xext_LIB Xext)
    if(NOT X11_Xext_LIB)
        message(FATAL_ERROR "Xext library not found.")
    endif()
    # 链接所有必需的X11库
    target_link_libraries(autogui-cpp PUBLIC
            ${X11_LIBRARIES}
            ${X11_Xtst_LIB}
            ${X11_Xext_LIB}
    )
    target_include_directories(autogui-cpp PUBLIC ${X11_INCLUDE_DIR})
endif()

# 集成方式: add_subdirectory
# 在你的项目CMakeLists.txt中:
#   add_subdirectory(autogui-cpp)
#   target_link_libraries(your_target PRIVATE autogui-cpp)
