# LAppLive2D — Live2D SDK 示例应用层，编译为静态库
# 依赖：Framework(libFramework.a) + Live2DCubismCore(libLive2DCubismCore.a)
# 无 Qt 依赖 — 完全解耦，仅依赖 OpenGL/GLES 头文件 + Cubism SDK

file(GLOB_RECURSE LAppLive2D_SOURCES
    CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/Src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Inc/*.hpp"
)

add_library(lapp_live2d STATIC ${LAppLive2D_SOURCES})

# =============================================
# 自身的公开头文件目录
# =============================================
target_include_directories(lapp_live2d
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/Inc"
)

# =============================================
# Live2D SDK 头文件（Framework + Core + stb）
# 这些是 lapp_live2d 编译时必需的，通过 PUBLIC 发布给最终链接目标
# =============================================
target_include_directories(lapp_live2d
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/../Framework/src"
        "${CMAKE_CURRENT_SOURCE_DIR}/../Core/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/../stb"
)

# =============================================
# 链接依赖 — 注意链接顺序！
# lapp_live2d → Framework → Live2DCubismCore
# Framework 内部使用了 Live2DCubismCore 的符号，所以 Core 必须在 Framework 之后
# lapp_live2d 无 Qt 依赖，开发者可自由选择 UI 后端(Qt/SDL/GLFW等)
# =============================================
target_link_libraries(lapp_live2d
    PUBLIC
        Framework           # Live2D Framework 静态库（IMPORTED，由父CMakeLists定义）
        Live2DCubismCore    # Live2D Core 静态库（IMPORTED，由父CMakeLists定义）
)
